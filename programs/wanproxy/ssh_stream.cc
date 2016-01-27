/*
 * Copyright (c) 2012-2016 Juli Mallett. All rights reserved.
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

#include <common/buffer.h>
#include <common/endian.h>
#include <common/thread/mutex.h>

#include <event/event_callback.h>

#include <io/pipe/pipe.h>
#include <io/socket/socket.h>
#include <io/pipe/splice.h>

#include <ssh/ssh_algorithm_negotiation.h>
#include <ssh/ssh_transport_pipe.h>

#include "ssh_proxy_config.h"
#include "ssh_stream.h"

#include "wanproxy_codec.h"

namespace {
	static const uint8_t SSHStreamPacket = 0xff;
}

SSHStream::SSHStream(const LogHandle& log, const SSHProxyConfig *ssh_config, SSH::Role role, WANProxyCodec *incoming_codec, WANProxyCodec *outgoing_codec)
: log_(log + "/ssh/stream/" + (role == SSH::ClientRole ? "client" : "server")),
  mtx_("SSHStream"),
  ssh_config_(ssh_config),
  socket_(NULL),
  session_(role),
  incoming_codec_(incoming_codec),
  outgoing_codec_(outgoing_codec),
  pipe_(NULL),
  splice_(NULL),
  splice_action_(NULL),
  start_cancel_(&mtx_, this, &SSHStream::start_cancel),
  start_callback_(NULL),
  start_action_(NULL),
  read_cancel_(&mtx_, this, &SSHStream::read_cancel),
  read_callback_(NULL),
  read_action_(NULL),
  input_buffer_(),
  ready_(false),
  ready_action_(NULL),
  write_cancel_(&mtx_, this, &SSHStream::write_cancel),
  write_callback_(NULL),
  write_action_(NULL)
{
	(void)outgoing_codec_;

	session_.algorithm_negotiation_ = new SSH::AlgorithmNegotiation(&session_);
	if (session_.role_ == SSH::ServerRole) {
		ASSERT_NON_NULL(log_, ssh_config_->server_host_key_);
		session_.algorithm_negotiation_->add_algorithm(ssh_config_->server_host_key_);
	}
	session_.algorithm_negotiation_->add_algorithms();

	pipe_ = new SSH::TransportPipe(&session_);

	ScopedLock _(&mtx_);
	SimpleCallback *cb = callback(&mtx_, this, &SSHStream::ready_complete);
	ready_action_ = pipe_->ready(cb);
}

SSHStream::~SSHStream()
{
	if (pipe_ != NULL) {
		delete pipe_;
		pipe_ = NULL;
	}

	ASSERT_NULL(log_, socket_);
	ASSERT_NULL(log_, splice_);
	ASSERT_NULL(log_, splice_action_);
	ASSERT_NULL(log_, start_callback_);
	ASSERT_NULL(log_, read_callback_);
	ASSERT_NULL(log_, read_action_);
}

Action *
SSHStream::start(Socket *socket, SimpleCallback *cb)
{
	ScopedLock _(&mtx_);
	socket_ = socket;
	start_callback_ = cb;

	splice_ = new Splice(log_ + "/splice", socket_, pipe_, socket_);
	EventCallback *scb = callback(&mtx_, this, &SSHStream::splice_complete);
	splice_action_ = splice_->start(scb);

	return (&start_cancel_);
}

Action *
SSHStream::close(SimpleCallback *cb)
{
	ScopedLock _(&mtx_);
	ASSERT_NULL(log_, start_callback_);
	ASSERT_NULL(log_, read_action_);
	ASSERT_NON_NULL(log_, socket_);

	/* Let our parent be responsible for closing the socket.  */
	socket_ = NULL;

	ASSERT_NULL(log_, start_action_);
	ASSERT_NULL(log_, start_callback_);

	return (cb->schedule());
}

Action *
SSHStream::read(size_t amt, EventCallback *cb)
{
	ScopedLock _(&mtx_);
	ASSERT_NULL(log_, read_action_);
	ASSERT_NULL(log_, read_callback_);

	if (pipe_ == NULL) {
		cb->param(Event::EOS);
		return (cb->schedule());
	}

	if (amt != 0) {
		cb->param(Event::Error);
		return (cb->schedule());
	}

	read_callback_ = cb;
	EventCallback *rcb = callback(&mtx_, this, &SSHStream::receive_complete);
	read_action_ = pipe_->receive(rcb);

	return (&read_cancel_);
}

Action *
SSHStream::write(Buffer *buf, EventCallback *cb)
{
	ScopedLock _(&mtx_);
	ASSERT_NULL(log_, write_action_);
	ASSERT_NULL(log_, write_callback_);

	ASSERT(log_, ready_ || input_buffer_.empty());

	buf->moveout(&input_buffer_);

	if (!ready_) {
		write_callback_ = cb;
		return (&write_cancel_);
	}

	write_do();

	cb->param(Event::Done);
	return (cb->schedule());
}

Action *
SSHStream::shutdown(bool, bool, EventCallback *cb)
{
	ScopedLock _(&mtx_);
	cb->param(Event::Error);
	return (cb->schedule());
}

void
SSHStream::start_cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
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
	ASSERT_NULL(log_, splice_action_);
	ASSERT_NULL(log_, splice_);
}

void
SSHStream::read_cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
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
	ASSERT_LOCK_OWNED(log_, &mtx_);
	splice_action_->cancel();
	splice_action_ = NULL;

	if (splice_ != NULL) {
		delete splice_;
		splice_ = NULL;
	}

	ASSERT_NON_NULL(log_, start_callback_);
	ASSERT_NULL(log_, start_action_);
	start_action_ = start_callback_->schedule();
	start_callback_ = NULL;
}

void
SSHStream::ready_complete(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	ready_action_->cancel();
	ready_action_ = NULL;

	ASSERT(log_, !ready_);
	ready_ = true;

	if (write_callback_ != NULL) {
		ASSERT_NULL(log_, write_action_);
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
	ASSERT_LOCK_OWNED(log_, &mtx_);
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
	ASSERT_LOCK_OWNED(log_, &mtx_);
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
	ASSERT_LOCK_OWNED(log_, &mtx_);
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

			Buffer packet;
			input_buffer_.moveout(&packet, sizeof length, length);

			pipe_->send(&packet);
		}
	}
}
