/*
 * Copyright (c) 2008-2013 Juli Mallett. All rights reserved.
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
#include <event/event_system.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_null.h>
#include <io/pipe/pipe_pair.h>
#include <io/socket/socket.h>
#include <io/pipe/splice.h>
#include <io/pipe/splice_pair.h>

#include <io/net/tcp_client.h>

#include "proxy_connector.h"

ProxyConnector::ProxyConnector(const std::string& name,
			 PipePair *pipe_pair, Socket *local_socket,
			 SocketImpl impl,
			 SocketAddressFamily family,
			 const std::string& remote_name)
: log_("/wanproxy/proxy/" + name + "/connector"),
  mtx_("ProxyConnector::" + name),
  stop_action_(NULL),
  local_action_(NULL),
  local_socket_(local_socket),
  remote_action_(NULL),
  remote_socket_(NULL),
  pipe_pair_(pipe_pair),
  incoming_pipe_(NULL),
  incoming_splice_(NULL),
  outgoing_pipe_(NULL),
  outgoing_splice_(NULL),
  splice_pair_(NULL),
  splice_action_(NULL)
{
	if (pipe_pair_ != NULL) {
		incoming_pipe_ = pipe_pair_->get_incoming();
		outgoing_pipe_ = pipe_pair_->get_outgoing();
	}

	ScopedLock _(&mtx_);
	SocketEventCallback *cb = callback(&mtx_, this, &ProxyConnector::connect_complete);
	remote_action_ = TCPClient::connect(impl, family, remote_name, cb);

	SimpleCallback *scb = callback(&mtx_, this, &ProxyConnector::stop);
	stop_action_ = EventSystem::instance()->register_interest(EventInterestStop, scb);
}

ProxyConnector::~ProxyConnector()
{
	ASSERT_NULL(log_, stop_action_);
	ASSERT_NULL(log_, local_action_);
	ASSERT_NULL(log_, local_socket_);
	ASSERT_NULL(log_, remote_action_);
	ASSERT_NULL(log_, remote_socket_);
	ASSERT_NULL(log_, incoming_splice_);
	ASSERT_NULL(log_, outgoing_splice_);
	ASSERT_NULL(log_, splice_pair_);
	ASSERT_NULL(log_, splice_action_);

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
ProxyConnector::local_close_complete(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	local_action_->cancel();
	local_action_ = NULL;

	ASSERT_NON_NULL(log_, local_socket_);
	delete local_socket_;
	local_socket_ = NULL;

	if (remote_socket_ == NULL)
		EventSystem::instance()->destroy(&mtx_, this);
}

void
ProxyConnector::remote_close_complete(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	remote_action_->cancel();
	remote_action_ = NULL;

	ASSERT_NON_NULL(log_, remote_socket_);
	delete remote_socket_;
	remote_socket_ = NULL;

	if (local_socket_ == NULL)
		EventSystem::instance()->destroy(&mtx_, this);
}


void
ProxyConnector::connect_complete(Event e, Socket *socket)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
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
	ASSERT_NON_NULL(log_, remote_socket_);

	incoming_splice_ = new Splice(log_ + "/incoming", local_socket_, incoming_pipe_, remote_socket_);
	outgoing_splice_ = new Splice(log_ + "/outgoing", remote_socket_, outgoing_pipe_, local_socket_);

	splice_pair_ = new SplicePair(outgoing_splice_, incoming_splice_);

	EventCallback *cb = callback(&mtx_, this, &ProxyConnector::splice_complete);
	splice_action_ = splice_pair_->start(cb);
}

void
ProxyConnector::splice_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
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
ProxyConnector::stop(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
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
ProxyConnector::schedule_close(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	if (stop_action_ != NULL) {
		stop_action_->cancel();
		stop_action_ = NULL;
	}

	if (splice_pair_ != NULL) {
		if (splice_action_ != NULL) {
			splice_action_->cancel();
			splice_action_ = NULL;
		}

		ASSERT_NON_NULL(log_, outgoing_splice_);
		ASSERT_NON_NULL(log_, incoming_splice_);

		delete splice_pair_;
		splice_pair_ = NULL;

		delete outgoing_splice_;
		outgoing_splice_ = NULL;

		delete incoming_splice_;
		incoming_splice_ = NULL;
	}

	ASSERT_NULL(log_, local_action_);
	ASSERT_NON_NULL(log_, local_socket_);
	SimpleCallback *lcb = callback(&mtx_, this, &ProxyConnector::local_close_complete);
	local_action_ = local_socket_->close(lcb);

	ASSERT_NULL(log_, remote_action_);
	if (remote_socket_ != NULL) {
		SimpleCallback *rcb = callback(&mtx_, this, &ProxyConnector::remote_close_complete);
		remote_action_ = remote_socket_->close(rcb);
	}
}
