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


#include <event/event_callback.h>

#include <io/io_uinet.h>
#include <io/socket/socket_uinet.h>

#include <uinet_api.h>


SocketUinet::SocketUinet(struct uinet_socket *so, int domain, int socktype, int protocol)
: Socket(domain, socktype, protocol),
  so_(so),
  log_("/socket/uinet"),
  scheduler_(IOUinet::instance()->scheduler()),
  accept_schedule_(false),
  accept_action_(NULL),
  accept_caller_callback_(NULL),
  accept_upcall_callback_(NULL)
{
	ASSERT(log_, so_ != NULL);
}


SocketUinet::~SocketUinet()
{
	ASSERT(log_, accept_schedule_ == false);
	ASSERT(log_, accept_action_ == NULL);
	ASSERT(log_, accept_caller_callback_ == NULL);
	ASSERT(log_, accept_upcall_callback_ == NULL);
}


int
SocketUinet::receive_upcall(void *arg, int wait_flag)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);

	(void)wait_flag;

	if (s->accept_schedule_) {
		s->accept_schedule_ = false;
		s->accept_upcall_callback_->param(Event(Event::Done));
		s->accept_action_ = s->accept_upcall_callback_->schedule();
	}

	return (UINET_SU_OK);
}


Action *
SocketUinet::accept(SocketEventCallback *cb)
{
	ASSERT(log_, accept_action_ == NULL);
	ASSERT(log_, accept_upcall_callback_ == NULL);
	ASSERT(log_, accept_caller_callback_ == NULL);

	accept_caller_callback_ = cb;
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
		accept_caller_callback_->param(Event::Done, child);
		accept_action_ = accept_caller_callback_->schedule();
		accept_caller_callback_ = NULL;
		break;
	}
	case UINET_EWOULDBLOCK:
		/*
		 * uinet_soaccept() invoked accept_wouldblock_handler() before returning.
		 */
		break;
	default:
		accept_caller_callback_->param(Event(Event::Error, error), NULL);
		accept_action_ = accept_caller_callback_->schedule();
		accept_caller_callback_ = NULL;;
		break;
	}
}


void
SocketUinet::accept_wouldblock_handler(void *arg)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);

	s->accept_schedule_ = true;

	/* XXX shouldn't this callback just be created once during SocketUinet construction? */
	s->accept_upcall_callback_ = callback(s->scheduler_, s, &SocketUinet::accept_callback);
}


void
SocketUinet::accept_callback(Event e)
{
	accept_action_->cancel();
	accept_action_ = NULL;

	delete accept_upcall_callback_;
	accept_upcall_callback_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::EOS:
		DEBUG(log_) << "Got EOS while waiting for accept: " << e;
		e.type_ = Event::Error;
		/* fallthrough */
	case Event::Error:
		accept_caller_callback_->param(e, NULL);
		accept_action_ = accept_caller_callback_->schedule();
		accept_caller_callback_ = NULL;
		return;
	default:
		HALT(log_) << "Unexpected event: " << e;
	}

	
	do_accept();
}


void
SocketUinet::accept_cancel(void)
{
	uinet_soupcall_lock(so_, UINET_SO_RCV);
	/* If waiting for upcall to schedule a callback, disable that. */
	if (accept_schedule_) {
		accept_schedule_ = false;
	}
	uinet_soupcall_unlock(so_, UINET_SO_RCV);

	if (accept_action_ != NULL) {
		accept_action_->cancel();
		accept_action_ = NULL;
	}

	if (accept_upcall_callback_ != NULL) {
		delete accept_upcall_callback_;
		accept_upcall_callback_ = NULL;
	}

	if (accept_caller_callback_ != NULL) {
		delete accept_caller_callback_;
		accept_caller_callback_ = NULL;
	}
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

	return (new SocketUinet(so, domainnum, typenum, protonum));
}
