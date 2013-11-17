/*
 * Copyright (c) 2013 Patrick Kelsey. All rights reserved.
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


#include <errno.h>

#include <event/event_callback.h>

#include <io/io_uinet.h>
#include <io/socket/resolver.h>
#include <io/socket/socket_uinet.h>

#include <uinet_api.h>


SocketUinet::SocketUinet(struct uinet_socket *so, int domain, int socktype, int protocol)
: Socket(domain, socktype, protocol),
  so_(so),
  log_("/socket/uinet"),
  scheduler_(IOUinet::instance()->scheduler()),
  accept_do_(false),
  accept_action_(NULL),
  accept_callback_(NULL),
  connect_do_(false),
  connect_action_(NULL),
  connect_callback_(NULL)
{
	ASSERT(log_, so_ != NULL);
}


SocketUinet::~SocketUinet()
{
	ASSERT(log_, accept_do_ == false);
	ASSERT(log_, accept_action_ == NULL);
	ASSERT(log_, accept_callback_ == NULL);
}


int
SocketUinet::receive_upcall(struct uinet_socket *so, void *arg, int wait_flag)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);
	
	(void)so;
	(void)wait_flag;

	if (s->accept_do_) {
		/*
		 * NB: accept_do_ needs to be cleared before calling
		 * accept_do() as accept_do() may need to set it again.
		 */
		s->accept_do_ = false;
		s->do_accept();
	}

	return (UINET_SU_OK);
}


Action *
SocketUinet::accept(SocketEventCallback *cb)
{
	/*
	 * These asserts enforce the requirement that once SockUinet::accept
	 * is invoked, it is not invoked again until the cancellation
	 * returned from the first invocation is canceled.
	 */
	ASSERT(log_, accept_do_ == false);
	ASSERT(log_, accept_action_ == NULL);
	ASSERT(log_, accept_callback_ == NULL);

	accept_callback_ = cb;
	do_accept();

	return (cancellation(this, &SocketUinet::accept_cancel));
}


void
SocketUinet::do_accept(void)
{
	uinet_socket *aso;

	int error = uinet_soaccept(so_, NULL, &aso);
	switch (error) {
	case 0: {
		SocketUinet *child = new SocketUinet(aso, domain_, socktype_, protocol_);
		accept_callback_->param(Event::Done, child);
		accept_action_ = accept_callback_->schedule();
		accept_callback_ = NULL;
		break;
	}
	case UINET_EWOULDBLOCK:
		/*
		 * uinet_soaccept() invoked accept_wouldblock_handler() before returning.
		 */
		break;
	default:
		accept_callback_->param(Event(Event::Error, uinet_errno_to_os(error)), NULL);
		accept_action_ = accept_callback_->schedule();
		accept_callback_ = NULL;;
		break;
	}
}


/*
 * Invoked from within uinet_soaccept when the result will be EWOULDBLOCK,
 * before the lock that provides mutual exlcusion with upcalls is released.
 */
void
SocketUinet::accept_wouldblock_handler(void *arg)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);

	s->accept_do_ = true;
}


void
SocketUinet::accept_cancel(void)
{
	uinet_soupcall_lock(so_, UINET_SO_RCV);
	/* If waiting for upcall to complete the work, disable that. */
	if (accept_do_) {
		accept_do_ = false;
	}
	uinet_soupcall_unlock(so_, UINET_SO_RCV);

	/*
	 * Because accept_do_ is guaranteed to be false at all times between
	 * the above lock release and the end of this routine, there is no
	 * chance that accept_action_ and accept_callback_ will be modified
	 * by the upcall while the code below executes.
	 */
	if (accept_action_ != NULL) {
		accept_action_->cancel();
		accept_action_ = NULL;
	}

	if (accept_callback_ != NULL) {
		delete accept_callback_;
		accept_callback_ = NULL;
	}
}


bool
SocketUinet::bind(const std::string& name)
{
	socket_address addr;

	if (!addr(domain_, socktype_, protocol_, name)) {
		ERROR(log_) << "Invalid name for bind: " << name;
		return (false);
	}

	int on = 1;
	int error = uinet_sosetsockopt(so_, UINET_SOL_SOCKET, UINET_SO_REUSEADDR, &on, sizeof on);
	if (error != 0) {
		ERROR(log_) << "Could not setsockopt(SO_REUSEADDR): " << strerror(uinet_errno_to_os(error));
	}

	/* XXX assuming that OS sockaddr internals are laid out the same as UINET sockaddr internals */
	error = uinet_sobind(so_, reinterpret_cast<struct uinet_sockaddr *>(&addr.addr_.sockaddr_));
	if (error != 0) {
		ERROR(log_) << "Could not bind: " << strerror(uinet_errno_to_os(error));
		return (false);
	}

	return (true);
}


int
SocketUinet::connect_upcall(struct uinet_socket *so, void *arg, int wait_flag)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);
	EventCallback *cb = s->connect_callback_;
	int state;
	
	(void)wait_flag;

	if (s->connect_do_) {
		state = uinet_sogetstate(s->so_);
	
		if (state & UINET_SS_ISCONNECTED) {
			cb->param(Event::Done);
			s->connect_action_ = cb->schedule();
			s->connect_do_ = false;

			uinet_soupcall_set(so, UINET_SO_RCV, SocketUinet::receive_upcall, s);
		} else if (state & UINET_SS_ISDISCONNECTED) {
			int error = uinet_sogeterror(s->so_);

			cb->param(Event(Event::Error, uinet_errno_to_os(error)));
			s->connect_action_ = cb->schedule();
			s->connect_do_ = false;
		}
	}

	return (UINET_SU_OK);
}


Action *
SocketUinet::connect(const std::string& name, EventCallback *cb)
{
	ASSERT(log_, connect_do_ == false);
	ASSERT(log_, connect_callback_ == NULL);
	ASSERT(log_, connect_action_ == NULL);

	socket_address addr;

	if (!addr(domain_, socktype_, protocol_, name)) {
		ERROR(log_) << "Invalid name for connect: " << name;
		cb->param(Event(Event::Error, EINVAL));
		return (cb->schedule());
	}

	connect_callback_ = cb;
	connect_do_ = true;

	/* XXX assuming that OS sockaddr internals are laid out the same as UINET sockaddr internals */
	int error = uinet_soconnect(so_, reinterpret_cast<struct uinet_sockaddr *>(&addr.addr_.sockaddr_));
	if (error != 0) {
		connect_do_ = false;
		cb->param(Event(Event::Error, uinet_errno_to_os(error)));
		connect_action_ = cb->schedule();
		connect_callback_ = NULL;
	}

	return (cancellation(this, &SocketUinet::connect_cancel));
}


void
SocketUinet::connect_cancel(void)
{
	uinet_soupcall_lock(so_, UINET_SO_RCV);
	if (connect_do_)
		connect_do_ = false;
	uinet_soupcall_lock(so_, UINET_SO_RCV);

	if (connect_action_ != NULL) {
		connect_action_->cancel();
		connect_action_ = NULL;
	}

	if (connect_callback_ != NULL) {
		delete connect_callback_;
		connect_callback_ = NULL;
	}
}


bool
SocketUinet::listen(void)
{
	/* XXX should have a knob to adjust somaxconn */
	int error = uinet_solisten(so_, 128);
	if (error != 0)
		return (false);
	return (true);
}


Action *
SocketUinet::shutdown(bool shut_read, bool shut_write, EventCallback *cb)
{
	int how;

	if (shut_read && shut_write)
		how = UINET_SHUT_RDWR;
	else if (shut_read)
		how = UINET_SHUT_RD;
	else if (shut_write)
		how = UINET_SHUT_WR;
	else {
		NOTREACHED(log_);
		return (NULL);
	}

	int error = uinet_soshutdown(so_, how);
	if (error != 0) {
		cb->param(Event(Event::Error, uinet_errno_to_os(error)));
		return (cb->schedule());
	}
	cb->param(Event::Done);
	return (cb->schedule());
}


Action *
SocketUinet::close(SimpleCallback *cb)
{
	int error = uinet_soclose(so_);
	if (error != 0) {
		/*
		 * We display the error here but do not pass it to the
		 * upper layers because it is almost impossible to get
		 * handling of close failing correct.
		 *
		 * For most errors, close fails because it already was
		 * closed by a peer or something like that, so it's as
		 * good as close succeeding.
		 */
		ERROR(log_) << "Close returned error: " << strerror(uinet_errno_to_os(error));
	}

	return (cb->schedule());
}


std::string
SocketUinet::getpeername(void) const
{
	socket_address sa;
	struct uinet_sockaddr *usa;

	int error = uinet_sogetpeeraddr(so_, &usa);
	if (error != 0)
		return ("<unknown>");

	/* XXX Check len.  */

	/* XXX assuming that OS sockaddr internals are laid out the same as UINET sockaddr internals */
	sa.addrlen_ = usa->sa_len;
	::memcpy(&sa.addr_.sockaddr_, usa, sa.addrlen_); 

	uinet_free_sockaddr(usa);

	return ((std::string)sa);
}

std::string
SocketUinet::getsockname(void) const
{
	socket_address sa;
	struct uinet_sockaddr *usa;

	int error = uinet_sogetsockaddr(so_, &usa);
	if (error != 0)
		return ("<unknown>");

	/* XXX Check len.  */

	/* XXX assuming that OS sockaddr internals are laid out the same as UINET sockaddr internals */
	sa.addrlen_ = usa->sa_len;
	::memcpy(&sa.addr_.sockaddr_, usa, sa.addrlen_); 

	uinet_free_sockaddr(usa);

	return ((std::string)sa);
}


SocketUinet *
SocketUinet::create(SocketAddressFamily family, SocketType type, const std::string& protocol, const std::string& hint)
{
	int typenum;

	switch (type) {
	case SocketTypeStream:
		typenum = UINET_SOCK_STREAM;
		break;

	case SocketTypeDatagram:
		typenum = UINET_SOCK_DGRAM;
		break;

	default:
		ERROR("/socket/uinet") << "Unsupported socket type.";
		return (NULL);
	}

	int protonum;

	if (protocol == "") {
		protonum = 0;
	} else {
		if (protocol == "tcp" || protocol == "TCP") {
			protonum = UINET_IPPROTO_TCP;
		} else if (protocol == "udp" || protocol == "UDP") {
			protonum = UINET_IPPROTO_UDP;
		} else {
			ERROR("/socket/uinet") << "Invalid protocol: " << protocol;
			return (NULL);
		}
	}

	int domainnum;

	switch (family) {
#if 0
	case SocketAddressFamilyIP:
		if (uinet_inet6_enabled()) {
			if (hint == "") {
				ERROR("/socket/uinet") << "Must specify hint address for IP sockets or specify IPv4 or IPv6 explicitly.";
				return (NULL);
			} else {
				/ * XXX evaluate hint */
				socket_address addr;

				if (!addr(UINET_AF_UNSPEC, typenum, protonum, hint)) {
					ERROR("/socket/uinet") << "Invalid hint: " << hint;
					return (NULL);
				}

				/* XXX Just make socket_address::operator() smarter about AF_UNSPEC?  */
				switch (addr.addr_.sockaddr_.sa_family) {
				case AF_INET:
					domainnum = UINET_AF_INET;
					break;

				case AF_INET6:
					domainnum = UINET_AF_INET6;
					break;

				default:
					ERROR("/socket/uinet") << "Unsupported address family for hint: " << hint;
					return (NULL);
				}
				break;
			}
		} else {
			(void)hint;
			domainnum = UINET_AF_INET;
		}
		break;
#endif

	case SocketAddressFamilyIPv4:
		domainnum = UINET_AF_INET;
		break;

	case SocketAddressFamilyIPv6:
		if (uinet_inet6_enabled()) {
			domainnum = UINET_AF_INET6;
		} else {
			ERROR("/socket/uinet") << "Unsupported address family.";
			return (NULL);
		}
		break;

	default:
		ERROR("/socket/uinet") << "Unsupported address family.";
		return (NULL);
	}

	struct uinet_socket *so;
	int error = uinet_socreate(domainnum, &so, typenum, protonum);
	if (error != 0) {
		/*
		 * If we were trying to create an IPv6 socket for a request that
		 * did not specify IPv4 vs. IPv6 and the system claims that the
		 * protocol is not supported, try explicitly creating an IPv4
		 * socket.
		 */
		if (uinet_inet6_enabled() && error == UINET_EPROTONOSUPPORT && domainnum == UINET_AF_INET6 &&
		    family == SocketAddressFamilyIP) {
			DEBUG("/socket/uinet") << "IPv6 socket create failed; trying IPv4.";
			return (SocketUinet::create(SocketAddressFamilyIPv4, type, protocol, hint));
		}

		ERROR("/socket/uinet") << "Could not create socket: " << strerror(uinet_errno_to_os(error));
		return (NULL);
	}

	uinet_sosetnonblocking(so, 1);

	return (new SocketUinet(so, domainnum, typenum, protonum));
}
