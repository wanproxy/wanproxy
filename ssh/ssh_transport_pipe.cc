/*
 * Copyright (c) 2011-2016 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <common/endian.h>
#include <common/thread/mutex.h>

#include <event/event_callback.h>

#include <http/http_protocol.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>

#include <ssh/ssh_algorithm_negotiation.h>
#include <ssh/ssh_compression.h>
#include <ssh/ssh_encryption.h>
#include <ssh/ssh_key_exchange.h>
#include <ssh/ssh_mac.h>
#include <ssh/ssh_protocol.h>
#include <ssh/ssh_session.h>
#include <ssh/ssh_transport_pipe.h>

namespace {
	static uint8_t zero_padding[255];
}

SSH::TransportPipe::TransportPipe(Session *session)
: PipeProducer("/ssh/transport/pipe", &mtx_),
  mtx_("SSH::TransportPipe"),
  session_(session),
  state_(GetIdentificationString),
  input_buffer_(),
  first_block_(),
  receive_cancel_(&mtx_, this, &TransportPipe::receive_cancel),
  receive_callback_(NULL),
  receive_action_(NULL),
  ready_(false),
  ready_cancel_(&mtx_, this, &TransportPipe::ready_cancel),
  ready_callback_(NULL),
  ready_action_(NULL)
{
	Buffer identification_string("SSH-2.0-WANProxy " + (std::string)log_);
	session_->local_version(identification_string);
	identification_string.append("\r\n");
	produce(&identification_string);
}

SSH::TransportPipe::~TransportPipe()
{
	ASSERT_NULL(log_, receive_callback_);
	ASSERT_NULL(log_, receive_action_);

	ASSERT_NULL(log_, ready_callback_);
	ASSERT_NULL(log_, ready_action_);
}

Action *
SSH::TransportPipe::receive(EventCallback *cb)
{
	ASSERT_NULL(log_, receive_callback_);
	ASSERT_NULL(log_, receive_action_);

	/*
	 * XXX
	 * This pattern is implemented about three different
	 * ways in the WANProxy codebase, and they are all kind
	 * of awful.  Need a better way to handle code that
	 * needs executed either on the request of the caller
	 * or when data comes in that satisfies a deferred
	 * callback to a caller.
	 *
	 * Would an EventDropbox suffice?  Or something like?
	 */
	receive_callback_ = cb;
	receive_do();

	if (receive_callback_ != NULL)
		return (&receive_cancel_);

	ASSERT_NON_NULL(log_, receive_action_);
	Action *a = receive_action_;
	receive_action_ = NULL;
	return (a);
}

/*
 * Because this is primarily for WAN optimization we always use minimal
 * padding and zero padding.  Quick and dirty.  Perhaps revisit later,
 * although it makes send() asynchronous unless we add a blocking
 * RNG interface.
 */
void
SSH::TransportPipe::send(Buffer *payload)
{
	Encryption *encryption_algorithm;
	MAC *mac_algorithm;
	Buffer packet;
	uint8_t padding_len;
	uint32_t packet_len;
	unsigned block_size;
	Buffer mac;

	ASSERT(log_, state_ == GetPacket);
	encryption_algorithm = session_->active_algorithms_.local_to_remote_->encryption_;
	if (encryption_algorithm != NULL) {
		block_size = encryption_algorithm->block_size();
		if (block_size < 8)
			block_size = 8;
	} else {
		block_size = 8;
	}
	mac_algorithm = session_->active_algorithms_.local_to_remote_->mac_;

	packet_len = sizeof padding_len + payload->length();
	padding_len = 4 + (block_size - ((sizeof packet_len + packet_len + 4) % block_size));
	packet_len += padding_len;

	BigEndian::append(&packet, packet_len);
	packet.append(padding_len);
	payload->moveout(&packet);
	packet.append(zero_padding, padding_len);

	if (mac_algorithm != NULL) {
		Buffer mac_input;

		SSH::UInt32::encode(&mac_input, session_->local_sequence_number_);
		mac_input.append(&packet);

		if (!mac_algorithm->mac(&mac, &mac_input)) {
			ERROR(log_) << "Could not compute outgoing MAC.";
			produce_error();
			return;
		}
	}

	if (encryption_algorithm != NULL) {
		Buffer ciphertext;
		if (!encryption_algorithm->cipher(&ciphertext, &packet)) {
			ERROR(log_) << "Could not encrypt outgoing packet.";
			produce_error();
			return;
		}
		packet = ciphertext;
	}
	if (!mac.empty())
		packet.append(mac);

	session_->local_sequence_number_++;

	produce(&packet);
}

Action *
SSH::TransportPipe::ready(SimpleCallback *cb)
{
	ASSERT_NULL(log_, ready_callback_);
	ASSERT_NULL(log_, ready_action_);

	if (ready_)
		return (cb->schedule());

	ready_callback_ = cb;

	return (&ready_cancel_);
}

void
SSH::TransportPipe::key_exchange_complete(void)
{
	if (!ready_) {
		ready_ = true;

		if (ready_callback_ != NULL) {
			ASSERT_NULL(log_, ready_action_);
			ready_action_ = ready_callback_->schedule();
			ready_callback_ = NULL;
		}
	} else {
		ASSERT_NULL(log_, ready_callback_);
	}
}

void
SSH::TransportPipe::consume(Buffer *in)
{
	/* XXX XXX XXX */
	if (in->empty()) {
		if (!input_buffer_.empty())
			DEBUG(log_) << "Received EOS with data outstanding.";
		produce_eos();
		return;
	}

	in->moveout(&input_buffer_);

	if (state_ == GetIdentificationString) {
		HTTPProtocol::ParseStatus status;

		while (!input_buffer_.empty()) {
			Buffer line;
			status = HTTPProtocol::ExtractLine(&line, &input_buffer_);
			switch (status) {
			case HTTPProtocol::ParseSuccess:
				break;
			case HTTPProtocol::ParseFailure:
				ERROR(log_) << "Invalid line while waiting for identification string.";
				produce_error();
				return;
			case HTTPProtocol::ParseIncomplete:
				/* Wait for more.  */
				return;
			}

			if (!line.prefix("SSH-"))
				continue; /* Next line.  */

			if (!line.prefix("SSH-2.0")) {
				ERROR(log_) << "Unsupported version.";
				produce_error();
				return;
			}

			session_->remote_version(line);

			state_ = GetPacket;
			/*
			 * XXX
			 * Should have a callback here?
			 */
			if (session_->algorithm_negotiation_ != NULL) {
				Buffer packet;
				if (session_->algorithm_negotiation_->init(&packet))
					send(&packet);
			}
			break;
		}
	}

	if (state_ == GetPacket && receive_callback_ != NULL)
		receive_do();
}

void
SSH::TransportPipe::receive_cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	if (receive_action_ != NULL) {
		receive_action_->cancel();
		receive_action_ = NULL;
	}

	if (receive_callback_ != NULL) {
		delete receive_callback_;
		receive_callback_ = NULL;
	}
}

void
SSH::TransportPipe::receive_do(void)
{
	ASSERT_NULL(log_, receive_action_);
	ASSERT_NON_NULL(log_, receive_callback_);

	if (state_ != GetPacket)
		return;

	while (!input_buffer_.empty()) {
		Encryption *encryption_algorithm;
		MAC *mac_algorithm;
		Buffer packet;
		Buffer mac;
		unsigned block_size;
		unsigned mac_size;
		uint32_t packet_len;
		uint8_t padding_len;
		uint8_t msg;

		encryption_algorithm = session_->active_algorithms_.remote_to_local_->encryption_;
		if (encryption_algorithm != NULL) {
			block_size = encryption_algorithm->block_size();
			if (block_size < 8)
				block_size = 8;
		} else {
			block_size = 8;
		}
		mac_algorithm = session_->active_algorithms_.remote_to_local_->mac_;
		if (mac_algorithm != NULL)
			mac_size = mac_algorithm->size();
		else
			mac_size = 0;

		if (input_buffer_.length() <= block_size) {
			DEBUG(log_) << "Waiting for first block of packet.";
			return;
		}

		if (encryption_algorithm != NULL) {
			if (first_block_.empty()) {
				Buffer block;
				input_buffer_.moveout(&block, block_size);
				if (!encryption_algorithm->cipher(&first_block_, &block)) {
					ERROR(log_) << "Decryption of first block failed.";
					produce_error();
					return;
				}
			}
			BigEndian::extract(&packet_len, &first_block_);
		} else {
			BigEndian::extract(&packet_len, &input_buffer_);
		}

		if (packet_len == 0) {
			ERROR(log_) << "Need to handle 0-length packet.";
			produce_error();
			return;
		}

		if (encryption_algorithm != NULL) {
			ASSERT(log_, !first_block_.empty());

			if (block_size + input_buffer_.length() < sizeof packet_len + packet_len + mac_size) {
				DEBUG(log_) << "Need " << sizeof packet_len + packet_len + mac_size << " bytes to decrypt encrypted packet; have " << (block_size + input_buffer_.length()) << ".";
				return;
			}

			first_block_.moveout(&packet);

			if (sizeof packet_len + packet_len > block_size) {
				Buffer ciphertext;
				input_buffer_.moveout(&ciphertext, sizeof packet_len + packet_len - block_size);
				if (!encryption_algorithm->cipher(&packet, &ciphertext)) {
					ERROR(log_) << "Decryption of packet failed.";
					produce_error();
					return;
				}
			} else {
				DEBUG(log_) << "Packet of exactly one block.";
			}
			ASSERT(log_, packet.length() == sizeof packet_len + packet_len);
		} else {
			if (input_buffer_.length() < sizeof packet_len + packet_len + mac_size) {
				DEBUG(log_) << "Need " << sizeof packet_len + packet_len + mac_size << " bytes; have " << input_buffer_.length() << ".";
				return;
			}

			input_buffer_.moveout(&packet, sizeof packet_len + packet_len);
		}

		if (mac_algorithm != NULL) {
			Buffer expected_mac;
			Buffer mac_input;

			input_buffer_.moveout(&mac, 0, mac_size);
			SSH::UInt32::encode(&mac_input, session_->remote_sequence_number_);
			mac_input.append(packet);
			if (!mac_algorithm->mac(&expected_mac, &mac_input)) {
				ERROR(log_) << "Could not compute expected MAC.";
				produce_error();
				return;
			}
			if (!expected_mac.equal(&mac)) {
				ERROR(log_) << "Received MAC does not match expected MAC.";
				produce_error();
				return;
			}
		}
		packet.skip(sizeof packet_len);

		session_->remote_sequence_number_++;

		padding_len = packet.pop();
		if (padding_len != 0) {
			if (packet.length() < padding_len) {
				ERROR(log_) << "Padding too large for packet.";
				produce_error();
				return;
			}
			packet.trim(padding_len);
		}

		if (packet.empty()) {
			ERROR(log_) << "Need to handle empty packet.";
			produce_error();
			return;
		}

		/*
		 * Pass by range to registered handlers for each range.
		 * Unhandled messages go to the receive_callback_, and
		 * the caller can register key exchange mechanisms,
		 * and handle (or discard) whatever they don't handle.
		 *
		 * NB: The caller could do all this, but it's assumed
		 *     that they usually have better things to do.  If
		 *     they register no handlers, they can certainly do
		 *     so by hand.
		 *
		 * XXX It seems like having a separate class which handles
		 *     all these details and algorithm negotiation would be
		 *     nice, and to have this one be a bit more oriented
		 *     towards managing just the transport layer.
		 *
		 *     At the very least, it needs to take responsibility
		 *     for its failures and allow the handler functions
		 *     here to mangle the packet buffer rather than trying
		 *     to send it on to the receiver if decoding fails.
		 *     A decoding failure should result in a disconnect,
		 *     an error.
		 */
		msg = packet.peek();
		if (msg >= SSH::Message::TransportRangeBegin &&
		    msg <= SSH::Message::TransportRangeEnd) {
			DEBUG(log_) << "Using default handler for transport message.";
		} else if (msg >= SSH::Message::AlgorithmNegotiationRangeBegin &&
			   msg <= SSH::Message::AlgorithmNegotiationRangeEnd) {
			if (session_->algorithm_negotiation_ != NULL) {
				if (session_->algorithm_negotiation_->input(this, &packet))
					continue;
				ERROR(log_) << "Algorithm negotiation message failed.";
				produce_error();
				return;
			}
			DEBUG(log_) << "Using default handler for algorithm negotiation message.";
		} else if (msg >= SSH::Message::KeyExchangeMethodRangeBegin &&
			   msg <= SSH::Message::KeyExchangeMethodRangeEnd) {
			if (session_->chosen_algorithms_.key_exchange_ != NULL) {
				if (session_->chosen_algorithms_.key_exchange_->input(this, &packet))
					continue;
				ERROR(log_) << "Key exchange message failed.";
				produce_error();
				return;
			}
			DEBUG(log_) << "Using default handler for key exchange method message.";
		} else if (msg >= SSH::Message::UserAuthenticationGenericRangeBegin &&
			   msg <= SSH::Message::UserAuthenticationGenericRangeEnd) {
			DEBUG(log_) << "Using default handler for generic user authentication message.";
		} else if (msg >= SSH::Message::UserAuthenticationMethodRangeBegin &&
			   msg <= SSH::Message::UserAuthenticationMethodRangeEnd) {
			DEBUG(log_) << "Using default handler for user authentication method message.";
		} else if (msg >= SSH::Message::ConnectionProtocolGlobalRangeBegin &&
			   msg <= SSH::Message::ConnectionProtocolGlobalRangeEnd) {
			DEBUG(log_) << "Using default handler for generic connection protocol message.";
		} else if (msg >= SSH::Message::ConnectionChannelRangeBegin &&
			   msg <= SSH::Message::ConnectionChannelRangeEnd) {
			DEBUG(log_) << "Using default handler for connection channel message.";
		} else if (msg >= SSH::Message::ClientProtocolReservedRangeBegin &&
			   msg <= SSH::Message::ClientProtocolReservedRangeEnd) {
			DEBUG(log_) << "Using default handler for client protocol message.";
		} else if (msg >= SSH::Message::LocalExtensionRangeBegin) {
			/* Because msg is a uint8_t, it will always be <= SSH::Message::LocalExtensionRangeEnd.  */
			DEBUG(log_) << "Using default handler for local extension message.";
		} else {
			ASSERT_ZERO(log_, msg);
			ERROR(log_) << "Message outside of protocol range received.  Passing to default handler, but not expecting much.";
		}

		receive_callback_->param(Event(Event::Done, packet));
		receive_action_ = receive_callback_->schedule();
		receive_callback_ = NULL;

		return;
	}
}

void
SSH::TransportPipe::ready_cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	if (ready_callback_ != NULL) {
		ASSERT(log_, !ready_);
		delete ready_callback_;
		ready_callback_ = NULL;
	}

	if (ready_action_ != NULL) {
		ASSERT(log_, ready_);
		ready_action_->cancel();
		ready_action_ = NULL;
	}
}
