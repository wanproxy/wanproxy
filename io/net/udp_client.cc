/*
 * Copyright (c) 2008-2016 Juli Mallett. All rights reserved.
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

#include <common/thread/mutex.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/socket.h>

#include <io/net/udp_client.h>

UDPClient::UDPClient(SocketImpl impl, SocketAddressFamily family)
: log_("/udp/client"),
  mtx_("UDPClient"),
  impl_(impl),
  family_(family),
  socket_(NULL),
  close_complete_(NULL, &mtx_, this, &UDPClient::close_complete),
  close_action_(NULL),
  connect_complete_(NULL, &mtx_, this, &UDPClient::connect_complete),
  connect_cancel_(&mtx_, this, &UDPClient::connect_cancel),
  connect_action_(NULL),
  connect_callback_(NULL)
{ }

UDPClient::~UDPClient()
{
	ASSERT_NULL(log_, connect_action_);
	ASSERT_NULL(log_, connect_callback_);
	ASSERT_NULL(log_, close_action_);

	if (socket_ != NULL) {
		delete socket_;
		socket_ = NULL;
	}
}

Action *
UDPClient::connect(const std::string& iface, const std::string& name, SocketEventCallback *ccb)
{
	ScopedLock _(&mtx_);
	ASSERT_NULL(log_, connect_action_);
	ASSERT_NULL(log_, connect_callback_);
	ASSERT_NULL(log_, socket_);

	socket_ = Socket::create(impl_, family_, SocketTypeDatagram, "udp", name);
	if (socket_ == NULL) {
		ccb->param(Event::Error, NULL);
		Action *a = ccb->schedule();

		delete this;

		return (a);
	}

	if (iface != "" && !socket_->bind(iface)) {
		ccb->param(Event::Error, NULL);
		Action *a = ccb->schedule();

#if 0
		/*
		 * XXX
		 * Need to schedule close and then delete ourselves.
		 */
		delete this;
#else
		HALT(log_) << "Socket bind failed.";
#endif

		return (a);
	}

	connect_action_ = socket_->connect(name, &connect_complete_);
	connect_callback_ = ccb;

	return (&connect_cancel_);
}

void
UDPClient::connect_cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	ASSERT_NULL(log_, close_action_);
	ASSERT_NON_NULL(log_, connect_action_);

	connect_action_->cancel();
	connect_action_ = NULL;

	if (connect_callback_ != NULL) {
		connect_callback_ = NULL;
	} else {
		/* XXX This has a race; caller could cancel after we schedule, but before callback occurs.  */
		/* Caller consumed Socket.  */
		socket_ = NULL;

		EventSystem::instance()->destroy(&mtx_, this);
		return;
	}

	ASSERT_NON_NULL(log_, socket_);
	close_action_ = socket_->close(&close_complete_);
}

void
UDPClient::connect_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	connect_action_->cancel();
	connect_action_ = NULL;

	connect_callback_->param(e, socket_);
	connect_action_ = connect_callback_->schedule();
	connect_callback_ = NULL;
}

void
UDPClient::close_complete(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	close_action_->cancel();
	close_action_ = NULL;

	delete this;
}

Action *
UDPClient::connect(SocketImpl impl, SocketAddressFamily family, const std::string& name, SocketEventCallback *cb)
{
	UDPClient *udp = new UDPClient(impl, family);
	return (udp->connect("", name, cb));
}

Action *
UDPClient::connect(SocketImpl impl, SocketAddressFamily family, const std::string& iface, const std::string& name, SocketEventCallback *cb)
{
	UDPClient *udp = new UDPClient(impl, family);
	return (udp->connect(iface, name, cb));
}
