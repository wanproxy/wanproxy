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

#include <sys/errno.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>

#include <common/endian.h>
#include <common/thread/mutex.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/resolver.h>
#include <io/socket/socket_handle.h>

/*
 * XXX Presently using AF_INET6 as the test for what is supported, but that is
 * wrong in many, many ways.
 */

SocketHandle::SocketHandle(int fd, int domain, int socktype, int protocol)
: Socket(domain, socktype, protocol),
  StreamHandle(fd),
  log_("/socket/handle"),
  mtx_("SocketHandle"),
  accept_action_(NULL),
  accept_callback_(NULL),
  connect_callback_(NULL),
  connect_action_(NULL)
{
	ASSERT(log_, fd_ != -1);
}

SocketHandle::~SocketHandle()
{
	ASSERT_NULL(log_, accept_action_);
	ASSERT_NULL(log_, accept_callback_);
	ASSERT_NULL(log_, connect_callback_);
	ASSERT_NULL(log_, connect_action_);
}

Action *
SocketHandle::accept(SocketEventCallback *cb)
{
	ScopedLock _(&mtx_);
	ASSERT_NULL(log_, accept_action_);
	ASSERT_NULL(log_, accept_callback_);

	accept_callback_ = cb;
	accept_action_ = accept_schedule();
	return (cancellation(&mtx_, this, &SocketHandle::accept_cancel));
}

bool
SocketHandle::bind(const std::string& name)
{
	ScopedLock _(&mtx_);
	socket_address addr;

	if (!addr(domain_, socktype_, protocol_, name)) {
		ERROR(log_) << "Invalid name for bind: " << name;
		return (false);
	}

	int on = 1;
	int rv = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
	if (rv == -1) {
		ERROR(log_) << "Could not setsockopt(SO_REUSEADDR): " << strerror(errno);
	}

	rv = ::bind(fd_, &addr.addr_.sockaddr_, addr.addrlen_);
	if (rv == -1) {
		ERROR(log_) << "Could not bind: " << strerror(errno);
		return (false);
	}

	return (true);
}

Action *
SocketHandle::connect(const std::string& name, EventCallback *cb)
{
	ScopedLock _(&mtx_);
	ASSERT_NULL(log_, connect_callback_);
	ASSERT_NULL(log_, connect_action_);

	socket_address addr;

	if (!addr(domain_, socktype_, protocol_, name)) {
		ERROR(log_) << "Invalid name for connect: " << name;
		cb->param(Event(Event::Error, EINVAL));
		return (cb->schedule());
	}

	/*
	 * TODO
	 *
	 * If the address we are connecting to is the address of another
	 * Socket, set up some sort of zero-copy IO between them.  This is
	 * easy enough if we do it right here.  Trying to do it at the file
	 * descriptor level seems to be really hard since there's a lot of
	 * races possible there.  This way, we're still at the point of the
	 * connection set up.
	 *
	 * We may want to allow the connection to complete so that calls to
	 * getsockname(2) and getpeername(2) do what you'd expect.  We may
	 * still want to poll on the file descriptors even, to make it
	 * possible to use tcpdrop, etc., to reset the connections.  I guess
	 * the thing to do is poll if there's no input ready.
	 */

	int rv = ::connect(fd_, &addr.addr_.sockaddr_, addr.addrlen_);
	switch (rv) {
	case 0:
		cb->param(Event::Done);
		connect_action_ = cb->schedule();
		break;
	case -1:
		switch (errno) {
		case EINPROGRESS:
			connect_callback_ = cb;
			connect_action_ = connect_schedule();
			break;
		default:
			cb->param(Event(Event::Error, errno));
			connect_action_ = cb->schedule();
			break;
		}
		break;
	default:
		HALT(log_) << "Connect returned unexpected value: " << rv;
	}
	return (cancellation(&mtx_, this, &SocketHandle::connect_cancel));
}

bool
SocketHandle::listen(void)
{
	ScopedLock _(&mtx_);
	int rv = ::listen(fd_, 128);
	if (rv == -1)
		return (false);
	return (true);
}

Action *
SocketHandle::shutdown(bool shut_read, bool shut_write, EventCallback *cb)
{
	ScopedLock _(&mtx_);
	int how;

	if (shut_read && shut_write)
		how = SHUT_RDWR;
	else if (shut_read)
		how = SHUT_RD;
	else if (shut_write)
		how = SHUT_WR;
	else {
		NOTREACHED(log_);
		return (NULL);
	}

	int rv = ::shutdown(fd_, how);
	if (rv == -1) {
		cb->param(Event(Event::Error, errno));
		return (cb->schedule());
	}
	cb->param(Event::Done);
	return (cb->schedule());
}

std::string
SocketHandle::getpeername(void) const
{
	/* XXX Read-lock?  */
	socket_address sa;
	int rv;

	sa.addrlen_ = sizeof sa.addr_;
	rv = ::getpeername(fd_, &sa.addr_.sockaddr_, &sa.addrlen_);
	if (rv == -1)
		return ("<unknown>");

	/* XXX Check len.  */

	return ((std::string)sa);
}

std::string
SocketHandle::getsockname(void) const
{
	/* XXX Read-lock?  */
	socket_address sa;
	int rv;

	sa.addrlen_ = sizeof sa.addr_;
	rv = ::getsockname(fd_, &sa.addr_.sockaddr_, &sa.addrlen_);
	if (rv == -1)
		return ("<unknown>");

	/* XXX Check len.  */

	return ((std::string)sa);
}

void
SocketHandle::accept_callback(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	accept_action_->cancel();
	accept_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::EOS:
		DEBUG(log_) << "Got EOS while waiting for accept: " << e;
		e.type_ = Event::Error;
	case Event::Error:
		accept_callback_->param(e, NULL);
		accept_action_ = accept_callback_->schedule();
		accept_callback_ = NULL;
		return;
	default:
		HALT(log_) << "Unexpected event: " << e;
	}

	int s = ::accept(fd_, NULL, NULL);
	if (s == -1) {
		switch (errno) {
		case EAGAIN:
			accept_action_ = accept_schedule();
			return;
		default:
			accept_callback_->param(Event(Event::Error, errno), NULL);
			accept_action_ = accept_callback_->schedule();
			accept_callback_ = NULL;
			return;
		}
	}

	Socket *child = new SocketHandle(s, domain_, socktype_, protocol_);
	accept_callback_->param(Event::Done, child);
	Action *a = accept_callback_->schedule();
	accept_action_ = a;
	accept_callback_ = NULL;
}

void
SocketHandle::accept_cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	ASSERT_NON_NULL(log_, accept_action_);
	accept_action_->cancel();
	accept_action_ = NULL;

	if (accept_callback_ != NULL) {
		delete accept_callback_;
		accept_callback_ = NULL;
	}
}

Action *
SocketHandle::accept_schedule(void)
{
	EventCallback *cb = callback(&mtx_, this, &SocketHandle::accept_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Readable, fd_, cb);
	return (a);
}

void
SocketHandle::connect_callback(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	connect_action_->cancel();
	connect_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		connect_callback_->param(Event::Done);
		break;
	case Event::EOS:
	case Event::Error:
		connect_callback_->param(Event(Event::Error, e.error_));
		break;
	default:
		HALT(log_) << "Unexpected event: " << e;
	}
	Action *a = connect_callback_->schedule();
	connect_action_ = a;
	connect_callback_ = NULL;
}

void
SocketHandle::connect_cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	ASSERT_NON_NULL(log_, connect_action_);
	connect_action_->cancel();
	connect_action_ = NULL;

	if (connect_callback_ != NULL) {
		delete connect_callback_;
		connect_callback_ = NULL;
	}
}

Action *
SocketHandle::connect_schedule(void)
{
	EventCallback *cb = callback(&mtx_, this, &SocketHandle::connect_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Writable, fd_, cb);
	return (a);
}

SocketHandle *
SocketHandle::create(SocketAddressFamily family, SocketType type, const std::string& protocol, const std::string& hint)
{
	int typenum;

	switch (type) {
	case SocketTypeStream:
		typenum = SOCK_STREAM;
		break;
	case SocketTypeDatagram:
		typenum = SOCK_DGRAM;
		break;
	default:
		ERROR("/socket/handle") << "Unsupported socket type.";
		return (NULL);
	}

	int protonum;

	if (protocol == "") {
		protonum = 0;
	} else {
		struct protoent *proto = getprotobyname(protocol.c_str());
		if (proto == NULL) {
			ERROR("/socket/handle") << "Invalid protocol: " << protocol;
			return (NULL);
		}
		protonum = proto->p_proto;
	}

	int domainnum;

	switch (family) {
	case SocketAddressFamilyIP:
#if defined(AF_INET6)
		if (hint == "") {
			ERROR("/socket/handle") << "Must specify hint address for IP sockets or specify IPv4 or IPv6 explicitly.";
			return (NULL);
		} else {
			socket_address addr;

			if (!addr(AF_UNSPEC, typenum, protonum, hint)) {
				ERROR("/socket/handle") << "Invalid hint: " << hint;
				return (NULL);
			}

			/* XXX Just make socket_address::operator() smarter about AF_UNSPEC?  */
			switch (addr.addr_.sockaddr_.sa_family) {
			case AF_INET:
				domainnum = AF_INET;
				break;
			case AF_INET6:
				domainnum = AF_INET6;
				break;
			default:
				ERROR("/socket/handle") << "Unsupported address family for hint: " << hint;
				return (NULL);
			}
			break;
		}
#else
		(void)hint;
		domainnum = AF_INET;
		break;
#endif
	case SocketAddressFamilyIPv4:
		domainnum = AF_INET;
		break;
#if defined(AF_INET6)
	case SocketAddressFamilyIPv6:
		domainnum = AF_INET6;
		break;
#endif
	case SocketAddressFamilyUnix:
		domainnum = AF_UNIX;
		break;
	default:
		ERROR("/socket/handle") << "Unsupported address family.";
		return (NULL);
	}

	int s = ::socket(domainnum, typenum, protonum);
	if (s == -1) {
#if defined(AF_INET6)
		/*
		 * If we were trying to create an IPv6 socket for a request that
		 * did not specify IPv4 vs. IPv6 and the system claims that the
		 * protocol is not supported, try explicitly creating an IPv4
		 * socket.
		 */
		if (errno == EPROTONOSUPPORT && domainnum == AF_INET6 &&
		    family == SocketAddressFamilyIP) {
			DEBUG("/socket/handle") << "IPv6 socket create failed; trying IPv4.";
			return (SocketHandle::create(SocketAddressFamilyIPv4, type, protocol, hint));
		}
#endif
		ERROR("/socket/handle") << "Could not create socket: " << strerror(errno);
		return (NULL);
	}

	return (new SocketHandle(s, domainnum, typenum, protonum));
}
