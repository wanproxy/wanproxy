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


#include <algorithm>

#include <errno.h>

#include <event/event_callback.h>

#include <io/io_uinet.h>
#include <io/socket/resolver.h>
#include <io/socket/socket_uinet.h>

#include <uinet_api.h>


#define UINET_READ_BUFFER_SIZE (64*1024)


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
  connect_callback_(NULL),
  read_do_(false),
  read_action_(NULL),
  read_callback_(NULL),
  read_amount_remaining_(0),
  read_buffer_(),
  write_do_(false),
  write_action_(NULL),
  write_callback_(NULL),
  write_amount_remaining_(0),
  write_buffer_()
{
	ASSERT(log_, so_ != NULL);
}


SocketUinet::~SocketUinet()
{
	ASSERT(log_, accept_do_ == false);
	ASSERT(log_, accept_action_ == NULL);
	ASSERT(log_, accept_callback_ == NULL);

	ASSERT(log_, connect_do_ == false);
	ASSERT(log_, connect_action_ == NULL);
	ASSERT(log_, connect_callback_ == NULL);

	ASSERT(log_, read_do_ == false);
	ASSERT(log_, read_action_ == NULL);
	ASSERT(log_, read_callback_ == NULL);
	ASSERT(log_, read_amount_remaining_ == 0);

	ASSERT(log_, write_do_ == false);
	ASSERT(log_, write_action_ == NULL);
	ASSERT(log_, write_callback_ == NULL);
	ASSERT(log_, write_amount_remaining_ == 0);
}


int
SocketUinet::passive_receive_upcall(struct uinet_socket *so, void *arg, int wait_flag)
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
		s->accept_do();
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
	accept_do();

	return (cancellation(this, &SocketUinet::accept_cancel));
}


void
SocketUinet::accept_do(void)
{
	uinet_socket *aso;

	int error = uinet_soaccept(so_, NULL, &aso);
	switch (error) {
	case 0: {
		SocketUinet *child = new SocketUinet(aso, domain_, socktype_, protocol_);

		uinet_sosetupcallprep(child->so_,
				      NULL, NULL,
				      SocketUinet::receive_upcall_prep, child,
				      SocketUinet::send_upcall_prep, child);

		uinet_soupcall_set(child->so_, UINET_SO_RCV, SocketUinet::active_receive_upcall, child);
		uinet_soupcall_set(child->so_, UINET_SO_SND, SocketUinet::active_send_upcall, child);

		accept_callback_->param(Event::Done, child);
		accept_action_ = accept_callback_->schedule();
		accept_callback_ = NULL;
		break;
	}
	case UINET_EWOULDBLOCK:
		/*
		 * uinet_soaccept() invoked accept_upcall_prep() before returning.
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
SocketUinet::accept_upcall_prep(struct uinet_socket *, void *arg)
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

			uinet_soupcall_set(so, UINET_SO_RCV, SocketUinet::active_receive_upcall, s);
			uinet_soupcall_set(so, UINET_SO_SND, SocketUinet::active_send_upcall, s);
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

	uinet_sosetupcallprep(so_,
			      NULL, NULL,
			      SocketUinet::receive_upcall_prep, this,
			      SocketUinet::send_upcall_prep, this);

	uinet_soupcall_set(so_, UINET_SO_RCV, SocketUinet::connect_upcall, this);

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
	uinet_soupcall_unlock(so_, UINET_SO_RCV);

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
	uinet_sosetupcallprep(so_,
			      SocketUinet::accept_upcall_prep, this,
			      NULL, NULL, NULL, NULL);

	uinet_soupcall_set(so_, UINET_SO_RCV, SocketUinet::passive_receive_upcall, this);

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


int
SocketUinet::active_receive_upcall(struct uinet_socket *so, void *arg, int wait_flag)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);
	
	(void)so;
	(void)wait_flag;

	if (s->read_do_) {
		/*
		 * NB: read_do_ needs to be cleared before calling read_do()
		 * as the callback scheduled by read_schedule() may need to
		 * set it again, and that callback will execute
		 * asynchronously to this context.
		 */
		s->read_do_ = false;
		s->read_schedule();
	}

	return (UINET_SU_OK);
}


Action *
SocketUinet::read(size_t amount, EventCallback *cb)
{
	ASSERT(log_, read_do_ == false);
	ASSERT(log_, read_callback_ == NULL);
	ASSERT(log_, read_action_ == NULL);
	ASSERT(log_, read_amount_remaining_ == 0);

	read_amount_remaining_ = amount;
	read_callback_ = cb;

	read_schedule();

	return (cancellation(this, &SocketUinet::read_cancel));
}


void
SocketUinet::read_schedule(void)
{
	ASSERT(log_, read_action_ == NULL);

	SimpleCallback *cb = callback(scheduler_, this, &SocketUinet::read_callback);
	read_action_ = cb->schedule();
}


void
SocketUinet::read_callback(void)
{
	read_action_->cancel();
	read_action_ = NULL;

	uint64_t read_amount = std::min(read_amount_remaining_, (uint64_t)UINET_READ_BUFFER_SIZE);

	struct uinet_iovec iov;
	struct uinet_uio uio;
	uint8_t read_data[UINET_READ_BUFFER_SIZE];

	uio.uio_iov = &iov;
	iov.iov_base = read_data;
	iov.iov_len = sizeof(read_data);
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_resid = read_amount;
	int error = uinet_soreceive(so_, NULL, &uio, NULL);
	
	uint64_t bytes_read = read_amount - uio.uio_resid;
	if (bytes_read > 0) {
		read_buffer_.append(read_data, bytes_read);
		read_amount_remaining_ -= bytes_read;
	}
	
	switch (error) {
	case 0:
		if (0 == bytes_read) {
			/*
			 * A zero-length result with no error indicates the other
			 * end of the socket has been closed, otherwise a
			 * zero-length result would be reported with EWOULDBLOCK.
			 */
			read_callback_->param(Event(Event::EOS, read_buffer_));
			read_action_ = read_callback_->schedule();
			read_callback_ = NULL;
			read_buffer_.clear();
			read_amount_remaining_ = 0;
		} else if (0 == read_amount_remaining_) {
			/*
			 * No more reads to do, so pass it all along.
			 */
			read_callback_->param(Event(Event::Done, read_buffer_));
			read_action_ = read_callback_->schedule();
			read_callback_ = NULL;
			read_buffer_.skip(bytes_read);
		} else if (0 == uio.uio_resid) {
			/*
			 * This individual read was completely fulfilled,
			 * but we have more to read to get to the total.
			 */
			read_schedule();
		}
		/*
		 * else
		 *
		 * This was a partial read due to lack of data. The next read
		 * will be scheduled during the next upcall (which may have
		 * happened already).
		 */
		break;
	case UINET_EWOULDBLOCK:
		/*
		 * There is not any data available currently, but the socket
		 * is still open and able to receive.  The next upcall will
		 * schedule the next read (and perhaps already has).
		 */
		break;
	default:
		/*
		 * Deliver the error along with whatever data we have.
		 */
		read_callback_->param(Event(Event::Error, uinet_errno_to_os(error), read_buffer_));
		read_action_ = read_callback_->schedule();
		read_callback_ = NULL;
		read_buffer_.clear();
		read_amount_remaining_ = 0;
		break;
	}
}


void
SocketUinet::receive_upcall_prep(struct uinet_socket *so, void *arg, int64_t resid)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);

	(void)so;
	(void)resid;

	s->read_do_ = true;
}


void
SocketUinet::read_cancel(void)
{
	uinet_soupcall_lock(so_, UINET_SO_RCV);
	if (read_do_)
		read_do_ = false;
	uinet_soupcall_unlock(so_, UINET_SO_RCV);

	/*
	 * Because read_do_ is guaranteed to be false at all times between
	 * the above lock release and the end of this routine, there is no
	 * chance that read_action_ will be modified by the upcall while the
	 * code below executes.
	 */
	if (read_action_ != NULL) {
		read_action_->cancel();
		read_action_ = NULL;
	}

	if (read_callback_ != NULL) {
		delete read_callback_;
		read_callback_ = NULL;
	}
}


int
SocketUinet::active_send_upcall(struct uinet_socket *so, void *arg, int wait_flag)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);
	
	(void)so;
	(void)wait_flag;

	if (s->write_do_) {
		/*
		 * NB: write_do_ needs to be cleared before calling write_do()
		 * as the callback scheduled by write_schedule() may need to
		 * set it again, and that callback will execute
		 * asynchronously to this context.
		 */
		s->write_do_ = false;
		s->write_schedule();
	}

	return (UINET_SU_OK);
}


Action *
SocketUinet::write(Buffer *buffer, EventCallback *cb)
{
	ASSERT(log_, write_do_ == false);
	ASSERT(log_, write_callback_ == NULL);
	ASSERT(log_, write_action_ == NULL);
	ASSERT(log_, write_buffer_.empty());

	buffer->moveout(&write_buffer_);
	write_callback_ = cb;

	write_schedule();

	return (cancellation(this, &SocketUinet::write_cancel));
}


void
SocketUinet::write_schedule(void)
{
	ASSERT(log_, write_action_ == NULL);

	SimpleCallback *cb = callback(scheduler_, this, &SocketUinet::write_callback);
	write_action_ = cb->schedule();
}


void
SocketUinet::write_callback(void)
{
	write_action_->cancel();
	write_action_ = NULL;

	struct iovec buf_iov[IOV_MAX];
	size_t iovcnt = write_buffer_.fill_iovec(buf_iov, std::min(IOV_MAX, UINET_IOV_MAX));
	ASSERT(log_, iovcnt != 0);

	struct uinet_iovec iov[UINET_IOV_MAX];
	uint64_t write_amount = 0;
	for (unsigned int i; i < iovcnt; i++) {
		iov[i].iov_base = buf_iov[i].iov_base;
		iov[i].iov_len = buf_iov[i].iov_len;
		write_amount += buf_iov[i].iov_len;
	}
	

	struct uinet_uio uio;

	uio.uio_iov = iov;
	uio.uio_iovcnt = iovcnt;
	uio.uio_offset = 0;
	uio.uio_resid = write_amount;
	int error = uinet_sosend(so_, NULL, &uio, 0);
	
	uint64_t bytes_written = write_amount - uio.uio_resid;
	if (bytes_written > 0) {
		write_buffer_.skip(bytes_written);
	}
	
	switch (error) {
	case 0:
		if (write_buffer_.empty()) {
			write_callback_->param(Event::Done);
			write_action_ = write_callback_->schedule();
			write_callback_ = NULL;
		} else if (0 == uio.uio_resid) {
			/*
			 * This individual write was completely performed,
			 * but we have more to write.
			 */
			write_schedule();
		}
		/*
		 * else
		 *
		 * This was a partial write due to lack of space. The next
		 * write will be scheduled during the next upcall (which may
		 * have happened already).
		 */
		break;
	case UINET_EWOULDBLOCK:
		/*
		 * Out of space, but the socket is still open and able to
		 * write.  The next upcall will schedule the next write (and
		 * perhaps already has).
		 */
		break;
	case UINET_EPIPE:
		/* XXX we can report the number of bytes written, if that's of any use */
		/* fallthrough */
	default:
		write_callback_->param(Event(Event::Error, uinet_errno_to_os(error)));
		write_action_ = write_callback_->schedule();
		write_callback_ = NULL;
		break;
	}
}


void
SocketUinet::send_upcall_prep(struct uinet_socket *so, void *arg, int64_t resid)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);

	(void)so;
	(void)resid;

	s->write_do_ = true;
}


void
SocketUinet::write_cancel(void)
{
	uinet_soupcall_lock(so_, UINET_SO_RCV);
	if (write_do_)
		write_do_ = false;
	uinet_soupcall_unlock(so_, UINET_SO_RCV);

	/*
	 * Because write_do_ is guaranteed to be false at all times between
	 * the above lock release and the end of this routine, there is no
	 * chance that write_action_ will be modified by the upcall while the
	 * code below executes.
	 */
	if (write_action_ != NULL) {
		write_action_->cancel();
		write_action_ = NULL;
	}

	if (write_callback_ != NULL) {
		delete write_callback_;
		write_callback_ = NULL;
	}
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
