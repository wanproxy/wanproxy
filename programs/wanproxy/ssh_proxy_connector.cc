/*
 * Copyright (c) 2008-2012 Juli Mallett. All rights reserved.
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

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_null.h>
#include <io/pipe/pipe_pair.h>
#include <io/socket/socket.h>
#include <io/pipe/splice.h>
#include <io/pipe/splice_pair.h>

#include <io/net/tcp_client.h>

#include "ssh_proxy_connector.h"

SSHProxyConnector::SSHProxyConnector(const std::string& name,
				     PipePair *pipe_pair, Socket *local_socket,
				     SocketAddressFamily family,
				     const std::string& remote_name,
				     WANProxyCodec *incoming_codec,
				     WANProxyCodec *outgoing_codec)
: log_("/wanproxy/proxy/" + name + "/connector"),
  stop_action_(NULL),
  local_action_(NULL),
  local_socket_(local_socket),
  remote_action_(NULL),
  remote_socket_(NULL),
  pipe_pair_(pipe_pair),
  incoming_stream_(log_, SSH::ServerRole, incoming_codec, outgoing_codec),
  incoming_pipe_(NULL),
  incoming_splice_(NULL),
  outgoing_stream_(log_, SSH::ClientRole, outgoing_codec, incoming_codec),
  outgoing_pipe_(NULL),
  outgoing_splice_(NULL),
  splice_pair_(NULL),
  splice_action_(NULL)
{
	if (pipe_pair_ != NULL) {
		incoming_pipe_ = pipe_pair_->get_incoming();
		outgoing_pipe_ = pipe_pair_->get_outgoing();
	}

	SocketEventCallback *cb = callback(this, &SSHProxyConnector::connect_complete);
	remote_action_ = TCPClient::connect(family, remote_name, cb);

	SimpleCallback *scb = callback(this, &SSHProxyConnector::stop);
	stop_action_ = EventSystem::instance()->register_interest(EventInterestStop, scb);
}

SSHProxyConnector::~SSHProxyConnector()
{
	ASSERT(log_, stop_action_ == NULL);
	ASSERT(log_, local_action_ == NULL);
	ASSERT(log_, local_socket_ == NULL);
	ASSERT(log_, remote_action_ == NULL);
	ASSERT(log_, remote_socket_ == NULL);
	ASSERT(log_, incoming_splice_ == NULL);
	ASSERT(log_, outgoing_splice_ == NULL);
	ASSERT(log_, splice_pair_ == NULL);
	ASSERT(log_, splice_action_ == NULL);

	if (pipe_pair_ != NULL) {
		delete pipe_pair_;
		pipe_pair_ = NULL;

		incoming_pipe_ = NULL;
		outgoing_pipe_ = NULL;
	}

	if (incoming_pipe_ != NULL) {
		delete incoming_pipe_;
		incoming_pipe_ = NULL;
	}

	if (outgoing_pipe_ != NULL) {
		delete outgoing_pipe_;
		outgoing_pipe_ = NULL;
	}
}

void
SSHProxyConnector::close_complete(Socket *socket)
{
	if (socket == local_socket_) {
		local_action_->cancel();
		local_action_ = NULL;
	}

	if (socket == remote_socket_) {
		remote_action_->cancel();
		remote_action_ = NULL;
	}

	if (socket == local_socket_) {
		ASSERT(log_, local_socket_ != NULL);
		delete local_socket_;
		local_socket_ = NULL;
	}

	if (socket == remote_socket_) {
		ASSERT(log_, remote_socket_ != NULL);
		delete remote_socket_;
		remote_socket_ = NULL;
	}

	if (local_socket_ == NULL && remote_socket_ == NULL) {
		delete this;
	}
}

void
SSHProxyConnector::connect_complete(Event e, Socket *socket)
{
	remote_action_->cancel();
	remote_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::Error:
		INFO(log_) << "Connect failed: " << e;
		schedule_close();
		return;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		schedule_close();
		return;
	}

	remote_socket_ = socket;
	ASSERT(log_, remote_socket_ != NULL);

	SimpleCallback *iscb = callback(this, &SSHProxyConnector::ssh_stream_complete, &incoming_stream_);
	incoming_stream_action_ = incoming_stream_.start(local_socket_, iscb);

	SimpleCallback *oscb = callback(this, &SSHProxyConnector::ssh_stream_complete, &outgoing_stream_);
	outgoing_stream_action_ = outgoing_stream_.start(remote_socket_, oscb);

	incoming_splice_ = new Splice(log_ + "/incoming", &incoming_stream_, incoming_pipe_, &outgoing_stream_);
	outgoing_splice_ = new Splice(log_ + "/outgoing", &outgoing_stream_, outgoing_pipe_, &incoming_stream_);

	splice_pair_ = new SplicePair(outgoing_splice_, incoming_splice_);

	EventCallback *cb = callback(this, &SSHProxyConnector::splice_complete);
	splice_action_ = splice_pair_->start(cb);
}

void
SSHProxyConnector::splice_complete(Event e)
{
	splice_action_->cancel();
	splice_action_ = NULL;

	delete splice_pair_;
	splice_pair_ = NULL;

	delete outgoing_splice_;
	outgoing_splice_ = NULL;

	delete incoming_splice_;
	incoming_splice_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		break;
	}

	schedule_close();
}

void
SSHProxyConnector::stop(void)
{
	stop_action_->cancel();
	stop_action_ = NULL;

	/*
	 * Connecting.
	 */
	if (local_action_ == NULL && remote_action_ != NULL &&
	    splice_action_ == NULL) {
		remote_action_->cancel();
		remote_action_ = NULL;

		schedule_close();
		return;
	}

	/*
	 * Already closing.  Should not happen.
	 */
	if (local_action_ != NULL || remote_action_ != NULL) {
		HALT(log_) << "Client already closing during stop.";
		return;
	}

	schedule_close();
}

void
SSHProxyConnector::schedule_close(void)
{
	if (stop_action_ != NULL) {
		stop_action_->cancel();
		stop_action_ = NULL;
	}

	if (splice_pair_ != NULL) {
		if (splice_action_ != NULL) {
			splice_action_->cancel();
			splice_action_ = NULL;
		}

		ASSERT(log_, outgoing_splice_ != NULL);
		ASSERT(log_, incoming_splice_ != NULL);

		delete splice_pair_;
		splice_pair_ = NULL;

		delete outgoing_splice_;
		outgoing_splice_ = NULL;

		delete incoming_splice_;
		incoming_splice_ = NULL;
	}

	ASSERT(log_, local_action_ == NULL);
	ASSERT(log_, local_socket_ != NULL);
	SimpleCallback *lcb = callback(this, &SSHProxyConnector::close_complete,
				       local_socket_);
	local_action_ = local_socket_->close(lcb);

	ASSERT(log_, remote_action_ == NULL);
	if (remote_socket_ != NULL) {
		SimpleCallback *rcb = callback(this, &SSHProxyConnector::close_complete,
					       remote_socket_);
		remote_action_ = remote_socket_->close(rcb);
	}
}

void
SSHProxyConnector::ssh_stream_complete(SSHStream *stream)
{
	if (stream == &incoming_stream_) {
		incoming_stream_action_->cancel();
		incoming_stream_action_ = NULL;
	} else {
		ASSERT(log_, stream == &outgoing_stream_);
		outgoing_stream_action_->cancel();
		outgoing_stream_action_ = NULL;
	}
}
