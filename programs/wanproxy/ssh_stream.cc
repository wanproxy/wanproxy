#include <common/buffer.h>
#include <common/endian.h>

#include <event/event_callback.h>

#include <io/pipe/pipe.h>
#include <io/socket/socket.h>
#include <io/pipe/splice.h>

#include <ssh/ssh_algorithm_negotiation.h>
#include <ssh/ssh_compression.h>
#include <ssh/ssh_encryption.h>
#include <ssh/ssh_language.h>
#include <ssh/ssh_mac.h>
#include <ssh/ssh_server_host_key.h>
#include <ssh/ssh_transport_pipe.h>

#include "ssh_stream.h"

#include "wanproxy_codec.h"

namespace {
	static const uint8_t SSHStreamPacket = 0xff;
}

SSHStream::SSHStream(const LogHandle& log, SSH::Role role, WANProxyCodec *incoming_codec, WANProxyCodec *outgoing_codec)
: log_(log + "/ssh/stream/" + (role == SSH::ClientRole ? "client" : "server")),
  socket_(NULL),
  session_(role),
  incoming_codec_(incoming_codec),
  outgoing_codec_(outgoing_codec),
  pipe_(NULL),
  splice_(NULL),
  splice_action_(NULL),
  start_callback_(NULL),
  start_action_(NULL),
  read_callback_(NULL),
  read_action_(NULL),
  input_buffer_(),
  ready_(false),
  ready_action_(NULL),
  write_callback_(NULL),
  write_action_(NULL)
{
	SSH::KeyExchange *key_exchange = SSH::KeyExchange::method(&session_);
	SSH::ServerHostKey *server_host_key;
	if (session_.role_ == SSH::ServerRole)
		server_host_key = SSH::ServerHostKey::server(&session_, "ssh-server1.pem");
	else
		server_host_key = SSH::ServerHostKey::client(&session_);
	SSH::Encryption *encryption = SSH::Encryption::cipher(CryptoEncryption::Cipher(CryptoEncryption::AES128, CryptoEncryption::CBC));
	SSH::MAC *mac = SSH::MAC::algorithm(CryptoMAC::SHA1);
	SSH::Compression *compression = SSH::Compression::none();
	SSH::Language *language = NULL;

	session_.algorithm_negotiation_ = new SSH::AlgorithmNegotiation(&session_, key_exchange, server_host_key,
									encryption, mac, compression, language);

	pipe_ = new SSH::TransportPipe(&session_);

	SimpleCallback *cb = callback(this, &SSHStream::ready_complete);
	ready_action_ = pipe_->ready(cb);
}

SSHStream::~SSHStream()
{
	if (pipe_ != NULL) {
		delete pipe_;
		pipe_ = NULL;
	}

	ASSERT(log_, socket_ == NULL);
	ASSERT(log_, splice_ == NULL);
	ASSERT(log_, splice_action_ == NULL);
	ASSERT(log_, start_callback_ == NULL);
	ASSERT(log_, read_callback_ == NULL);
	ASSERT(log_, read_action_ == NULL);
}

Action *
SSHStream::start(Socket *socket, SimpleCallback *cb)
{
	socket_ = socket;
	start_callback_ = cb;

	splice_ = new Splice(log_ + "/splice", socket_, pipe_, socket_);
	EventCallback *scb = callback(this, &SSHStream::splice_complete);
	splice_action_ = splice_->start(scb);

	return (cancellation(this, &SSHStream::start_cancel));
}

Action *
SSHStream::close(SimpleCallback *cb)
{
	ASSERT(log_, start_callback_ == NULL);
	ASSERT(log_, read_action_ == NULL);
	ASSERT(log_, socket_ != NULL);

	/* Let our parent be responsible for closing the socket.  */
	socket_ = NULL;

	ASSERT(log_, start_action_ == NULL);
	ASSERT(log_, start_callback_ == NULL);

	return (cb->schedule());
}

Action *
SSHStream::read(size_t amt, EventCallback *cb)
{
	ASSERT(log_, read_action_ == NULL);
	ASSERT(log_, read_callback_ == NULL);

	if (pipe_ == NULL) {
		cb->param(Event::EOS);
		return (cb->schedule());
	}

	if (amt != 0) {
		cb->param(Event::Error);
		return (cb->schedule());
	}

	read_callback_ = cb;
	EventCallback *rcb = callback(this, &SSHStream::receive_complete);
	read_action_ = pipe_->receive(rcb);

	return (cancellation(this, &SSHStream::read_cancel));
}

Action *
SSHStream::write(Buffer *buf, EventCallback *cb)
{
	ASSERT(log_, write_action_ == NULL);
	ASSERT(log_, write_callback_ == NULL);

	ASSERT(log_, ready_ || input_buffer_.empty());

	buf->moveout(&input_buffer_);

	if (!ready_) {
		write_callback_ = cb;
		return (cancellation(this, &SSHStream::write_cancel));
	}

	write_do();

	cb->param(Event::Done);
	return (cb->schedule());
}

Action *
SSHStream::shutdown(bool, bool, EventCallback *cb)
{
	cb->param(Event::Error);
	return (cb->schedule());
}

void
SSHStream::start_cancel(void)
{
	if (start_callback_ != NULL) {
		delete start_callback_;
		start_callback_ = NULL;
	}
	
	if (start_action_ != NULL) {
		start_action_->cancel();
		start_action_ = NULL;
	}

	if (splice_action_ != NULL) {
		splice_action_->cancel();
		splice_action_ = NULL;

		delete splice_;
		splice_ = NULL;
	}
	ASSERT(log_, splice_action_ == NULL);
	ASSERT(log_, splice_ == NULL);
}

void
SSHStream::read_cancel(void)
{
	if (read_callback_ != NULL) {
		delete read_callback_;
		read_callback_ = NULL;
	}

	if (read_action_ != NULL) {
		read_action_->cancel();
		read_action_ = NULL;
	}
}

void
SSHStream::splice_complete(Event)
{
	splice_action_->cancel();
	splice_action_ = NULL;

	if (splice_ != NULL) {
		delete splice_;
		splice_ = NULL;
	}

	ASSERT(log_, start_callback_ != NULL);
	ASSERT(log_, start_action_ == NULL);
	start_action_ = start_callback_->schedule();
	start_callback_ = NULL;
}

void
SSHStream::ready_complete(void)
{
	ready_action_->cancel();
	ready_action_ = NULL;

	ASSERT(log_, !ready_);
	ready_ = true;

	if (write_callback_ != NULL) {
		ASSERT(log_, write_action_ == NULL);
		ASSERT(log_, !input_buffer_.empty());

		write_do();

		write_callback_->param(Event::Done);
		write_action_ = write_callback_->schedule();
		write_callback_ = NULL;
	}
}

void
SSHStream::receive_complete(Event e)
{
	read_action_->cancel();
	read_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
		break;
	case Event::Error:
		ERROR(log_) << "Error during receive: " << e;
		read_callback_->param(e);
		read_action_ = read_callback_->schedule();
		read_callback_ = NULL;
		return;
	default:
		HALT(log_) << "Unexpected event: " << e;
	}

	if (!e.buffer_.empty()) {
		/*
		 * If we're writing data that has been encoded, we need to untag it.
		 * Otherwise we need to frame it.
		 */
		if (incoming_codec_ != NULL &&
		    (incoming_codec_->codec_ != NULL || incoming_codec_->compressor_)) {
			if (e.buffer_.peek() != SSHStreamPacket ||
			    e.buffer_.length() == 1) {
				ERROR(log_) << "Got encoded packet with wrong message.";
				read_callback_->param(Event::Error);
				read_action_ = read_callback_->schedule();
				read_callback_ = NULL;
				return;
			}
			e.buffer_.skip(1);
		} else {
			uint32_t length = e.buffer_.length();
			length = BigEndian::encode(length);

			Buffer buf;
			buf.append(&length);
			e.buffer_.moveout(&buf);
			e.buffer_ = buf;
		}
	}

	read_callback_->param(e);
	read_action_ = read_callback_->schedule();
	read_callback_ = NULL;
}

void
SSHStream::write_cancel(void)
{
	if (write_action_ != NULL) {
		write_action_->cancel();
		write_action_ = NULL;
	}

	if (write_callback_ != NULL) {
		delete write_callback_;
		write_callback_ = NULL;
	}
}

void
SSHStream::write_do(void)
{
	/*
	 * If we're writing data that has been encoded, we need to tag it.
	 */
	if (incoming_codec_ != NULL &&
	    (incoming_codec_->codec_ != NULL || incoming_codec_->compressor_)) {
		Buffer packet;
		packet.append(SSHStreamPacket);
		input_buffer_.moveout(&packet);
		pipe_->send(&packet);
	} else {
		uint32_t length;
		while (input_buffer_.length() > sizeof length) {
			input_buffer_.extract(&length);
			length = BigEndian::decode(length);

			if (input_buffer_.length() < sizeof length + length) {
				DEBUG(log_) << "Waiting for more write data.";
				break;
			}

			input_buffer_.skip(sizeof length);
			Buffer packet;
			input_buffer_.moveout(&packet, length);

			pipe_->send(&packet);
		}
	}
}
