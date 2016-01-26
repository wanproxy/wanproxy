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

#include <event/cancellation.h>
#include <event/event_callback.h>

#include <io/io_uinet.h>
#include <io/socket/resolver.h>
#include <io/socket/socket_uinet.h>

#include <uinet_api.h>

#define UINET_READ_BUFFER_SIZE (64*1024)

SocketUinet::SocketUinet(struct uinet_socket *so, int domain, int socktype, int protocol)
: Socket(domain, socktype, protocol),
  log_("/socket/uinet"),
  scheduler_(IOUinet::instance()->scheduler()),
  accept_action_(NULL),
  accept_callback_(NULL),
  accept_connect_mtx_("SocketUinet::connect/accept"),
  connect_action_(NULL),
  connect_callback_(NULL),
  read_action_(NULL),
  read_callback_(NULL),
  read_amount_remaining_(0),
  read_buffer_(),
  read_mtx_("SocketUinet::read"),
  write_action_(NULL),
  write_callback_(NULL),
  write_amount_remaining_(0),
  write_buffer_(),
  write_mtx_("SocketUinet::write"),
  upcall_mtx_("SocketUinet::upcall"),
  upcall_action_(NULL),
  upcall_pending_(),
  so_(so)
{
	ASSERT_NON_NULL(log_, so_);
}

SocketUinet::~SocketUinet()
{
	ASSERT_NULL(log_, accept_action_);
	ASSERT_NULL(log_, accept_callback_);

	ASSERT_NULL(log_, connect_action_);
	ASSERT_NULL(log_, connect_callback_);

	ASSERT_NULL(log_, read_action_);
	ASSERT_NULL(log_, read_callback_);
	ASSERT_ZERO(log_, read_amount_remaining_);

	ASSERT_NULL(log_, write_action_);
	ASSERT_NULL(log_, write_callback_);
	ASSERT_ZERO(log_, write_amount_remaining_);

	ASSERT_NULL(log_, upcall_action_);
}

int
SocketUinet::passive_receive_upcall(struct uinet_socket *, void *arg, int)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);
	
	s->upcall_schedule(SOCKET_UINET_UPCALL_PASSIVE_RECEIVE);

	return (UINET_SU_OK);
}

Action *
SocketUinet::accept(SocketEventCallback *cb)
{
	ScopedLock _(&accept_connect_mtx_);

	ASSERT_NULL(log_, accept_action_);
	ASSERT_NULL(log_, accept_callback_);

	accept_callback_ = cb;

	uinet_soupcall_set(so_, UINET_SO_RCV, passive_receive_upcall, this);

	accept_schedule();

	return (cancellation(&accept_connect_mtx_, this, &SocketUinet::accept_cancel));
}

void
SocketUinet::accept_schedule(void)
{
	if (accept_action_ != NULL)
		return;

	SimpleCallback *cb = callback(scheduler_, &accept_connect_mtx_, this, &SocketUinet::accept_callback);
	accept_action_ = cb->schedule();
}

void
SocketUinet::accept_callback(void)
{
	ASSERT_LOCK_OWNED(log_, &accept_connect_mtx_);
	ASSERT_NON_NULL(log_, accept_action_);
	accept_action_->cancel();
	accept_action_ = NULL;

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
		break;
	default:
		accept_callback_->param(Event(Event::Error, uinet_errno_to_os(error)), NULL);
		accept_action_ = accept_callback_->schedule();
		accept_callback_ = NULL;;
		break;
	}
}

void
SocketUinet::accept_cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &accept_connect_mtx_);

	if (accept_action_ != NULL) {
		accept_action_->cancel();
		accept_action_ = NULL;
	}

	if (accept_callback_ != NULL) {
		delete accept_callback_;
		accept_callback_ = NULL;
	}

	uinet_soupcall_clear(so_, UINET_SO_RCV);
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
SocketUinet::connect_upcall(struct uinet_socket *, void *arg, int)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);
	
	s->upcall_schedule(SOCKET_UINET_UPCALL_CONNECT);

	return (UINET_SU_OK);
}

Action *
SocketUinet::connect(const std::string& name, EventCallback *cb)
{
	ScopedLock _(&accept_connect_mtx_);

	ASSERT_NULL(log_, connect_callback_);
	ASSERT_NULL(log_, connect_action_);

	socket_address addr;

	if (!addr(domain_, socktype_, protocol_, name)) {
		ERROR(log_) << "Invalid name for connect: " << name;
		cb->param(Event(Event::Error, EINVAL));
		return (cb->schedule());
	}

	/* XXX assuming that OS sockaddr internals are laid out the same as UINET sockaddr internals */
	int error = uinet_soconnect(so_, reinterpret_cast<struct uinet_sockaddr *>(&addr.addr_.sockaddr_));
	switch (error) {
	case 0:
	case UINET_EINPROGRESS:
		/*
		 * A zero return indicates the connection is already
		 * complete.  UINET_INPROGRESS indicates the connection is
		 * being attempted but has not yet completed.  In either
		 * case, the upcall will schedule the callback.
		 */
		break;
	default:
		cb->param(Event(Event::Error, uinet_errno_to_os(error)));
		return (cb->schedule());
	}

	connect_callback_ = cb;

	uinet_soupcall_set(so_, UINET_SO_SND, connect_upcall, this);

	connect_schedule();

	return (cancellation(&accept_connect_mtx_, this, &SocketUinet::connect_cancel));
}

void
SocketUinet::connect_schedule(void)
{
	if (connect_action_ != NULL)
		return;

	SimpleCallback *cb = callback(scheduler_, &accept_connect_mtx_, this, &SocketUinet::connect_callback);
	connect_action_ = cb->schedule();
}

void
SocketUinet::connect_callback(void)
{
	ASSERT_LOCK_OWNED(log_, &accept_connect_mtx_);
	ASSERT_NON_NULL(log_, connect_action_);
	connect_action_->cancel();
	connect_action_ = NULL;

	int state = uinet_sogetstate(so_);
	if (state & UINET_SS_ISCONNECTED) {
		connect_callback_->param(Event::Done);
		connect_action_ = connect_callback_->schedule();
		connect_callback_ = NULL;
	} else if (state & UINET_SS_ISDISCONNECTED) {
		int error = uinet_sogeterror(so_);

		connect_callback_->param(Event(Event::Error, uinet_errno_to_os(error)));
		connect_action_ = connect_callback_->schedule();
		connect_callback_ = NULL;
	} else {
		DEBUG(log_) << "Connection in progress.";
	}
}

void
SocketUinet::connect_cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &accept_connect_mtx_);

	if (connect_action_ != NULL) {
		connect_action_->cancel();
		connect_action_ = NULL;
	}

	if (connect_callback_ != NULL) {
		delete connect_callback_;
		connect_callback_ = NULL;
	}

	uinet_soupcall_clear(so_, UINET_SO_SND);
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
	ASSERT_NON_NULL(log_, so_);

	ASSERT_NULL(log_, accept_action_);
	ASSERT_NULL(log_, accept_callback_);
	ASSERT_NULL(log_, connect_action_);
	ASSERT_NULL(log_, connect_callback_);
	ASSERT_NULL(log_, read_action_);
	ASSERT_NULL(log_, read_callback_);
	ASSERT_NULL(log_, write_action_);
	ASSERT_NULL(log_, write_callback_);

	/*
	 * These ought to be unnecessary, but can
	 * we replace them with assertions?
	 */
	uinet_soupcall_clear(so_, UINET_SO_RCV);
	uinet_soupcall_clear(so_, UINET_SO_SND);

	/*
	 * The upcalls have been disabled, cancel any upcall callbacks.
	 */
	{
		ScopedLock _(&upcall_mtx_);
		if (upcall_action_ != NULL) {
			upcall_action_->cancel();
			upcall_action_ = NULL;
		}
	}

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
	so_ = NULL;

	return (cb->schedule());
}

int
SocketUinet::active_receive_upcall(struct uinet_socket *, void *arg, int)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);
	
	s->upcall_schedule(SOCKET_UINET_UPCALL_ACTIVE_RECEIVE);

	return (UINET_SU_OK);
}

Action *
SocketUinet::read(size_t amount, EventCallback *cb)
{
	ScopedLock _(&read_mtx_);

	ASSERT_NULL(log_, read_callback_);
	ASSERT_NULL(log_, read_action_);
	ASSERT_ZERO(log_, read_amount_remaining_);

	read_amount_remaining_ = amount;
	read_callback_ = cb;

	uinet_soupcall_set(so_, UINET_SO_RCV, active_receive_upcall, this);

	read_do();

	if (read_callback_ == NULL) {
		ASSERT_NON_NULL(log_, read_action_);
		Action *a = read_action_;
		read_action_ = NULL;
		return (a);
	}

	/* Need to wait for deferred read to occur.  */
	return (cancellation(&read_mtx_, this, &SocketUinet::read_cancel));
}

void
SocketUinet::read_schedule(void)
{
	if (read_action_ != NULL)
		return;

	SimpleCallback *cb = callback(scheduler_, &read_mtx_, this, &SocketUinet::read_callback);
	read_action_ = cb->schedule();
}

void
SocketUinet::read_callback(void)
{
	ASSERT_LOCK_OWNED(log_, &read_mtx_);
	/*
	 * We have already been cancelled, but lost the race.
	 */
	if (read_action_ == NULL)
		return;
	read_action_->cancel();
	read_action_ = NULL;

	read_do();
}

void
SocketUinet::read_do(void)
{
	uint64_t read_amount;

	/*
	 * If read_amount_remaining_ is zero here, it means we are servicing
	 * a zero-length read.  A zero-length read is serviced by providing
	 * whatever data is currently available, up to
	 * UINET_READ_BUFFER_SIZE, to the caller, or, if there is no data
	 * currently available, scheduling the aforementioned action to
	 * occur as soon as data becomes available.
	 */
	if (0 == read_amount_remaining_)
		read_amount = UINET_READ_BUFFER_SIZE;
	else
		read_amount = std::min(read_amount_remaining_, (uint64_t)UINET_READ_BUFFER_SIZE);

	struct uinet_iovec iov;
	struct uinet_uio uio;
	uint8_t read_data[UINET_READ_BUFFER_SIZE];

	uio.uio_iov = &iov;
	iov.iov_base = read_data;
	iov.iov_len = sizeof(read_data);
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_resid = read_amount;
	int flags = UINET_MSG_DONTWAIT | UINET_MSG_NBIO;
	int error = uinet_soreceive(so_, NULL, &uio, &flags);
	
	uint64_t bytes_read = read_amount - uio.uio_resid;
	if (bytes_read > 0) {
		read_buffer_.append(read_data, bytes_read);
		if (read_amount_remaining_)
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
			ASSERT_NULL(log_, read_action_);
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
SocketUinet::read_cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &read_mtx_);

	if (read_action_ != NULL) {
		read_action_->cancel();
		read_action_ = NULL;
	}

	if (read_callback_ != NULL) {
		delete read_callback_;
		read_callback_ = NULL;
	}

	uinet_soupcall_clear(so_, UINET_SO_RCV);
}

int
SocketUinet::active_send_upcall(struct uinet_socket *, void *arg, int)
{
	SocketUinet *s = static_cast<SocketUinet *>(arg);

	s->upcall_schedule(SOCKET_UINET_UPCALL_ACTIVE_SEND);

	return (UINET_SU_OK);
}

Action *
SocketUinet::write(Buffer *buffer, EventCallback *cb)
{
	ScopedLock _(&write_mtx_);

	ASSERT_NULL(log_, write_callback_);
	ASSERT_NULL(log_, write_action_);
	ASSERT(log_, write_buffer_.empty());

	buffer->moveout(&write_buffer_);
	write_callback_ = cb;

	uinet_soupcall_set(so_, UINET_SO_SND, active_send_upcall, this);

	write_schedule();

	return (cancellation(&write_mtx_, this, &SocketUinet::write_cancel));
}

void
SocketUinet::write_schedule(void)
{
	if (write_action_ != NULL)
		return;

	SimpleCallback *cb = callback(scheduler_, &write_mtx_, this, &SocketUinet::write_callback);
	write_action_ = cb->schedule();
}

void
SocketUinet::write_callback(void)
{
	ASSERT_LOCK_OWNED(log_, &write_mtx_);
	ASSERT_NON_NULL(log_, write_action_);
	write_action_->cancel();
	write_action_ = NULL;

	struct iovec buf_iov[IOV_MAX];
	size_t iovcnt = write_buffer_.fill_iovec(buf_iov, std::min(IOV_MAX, UINET_IOV_MAX));
	ASSERT_NON_ZERO(log_, iovcnt);

	struct uinet_iovec iov[UINET_IOV_MAX];
	uint64_t write_amount = 0;
	for (unsigned int i = 0; i < iovcnt; i++) {
		iov[i].iov_base = buf_iov[i].iov_base;
		iov[i].iov_len = buf_iov[i].iov_len;
		write_amount += buf_iov[i].iov_len;
	}
	
	struct uinet_uio uio;

	uio.uio_iov = iov;
	uio.uio_iovcnt = iovcnt;
	uio.uio_offset = 0;
	uio.uio_resid = write_amount;
	int error = uinet_sosend(so_, NULL, &uio, UINET_MSG_DONTWAIT | UINET_MSG_NBIO);
	
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
			ASSERT_NULL(log_, write_action_);
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
SocketUinet::write_cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &write_mtx_);

	if (write_action_ != NULL) {
		write_action_->cancel();
		write_action_ = NULL;
	}

	if (write_callback_ != NULL) {
		delete write_callback_;
		write_callback_ = NULL;
	}

	uinet_soupcall_clear(so_, UINET_SO_SND);
}

void
SocketUinet::upcall_schedule(unsigned kind)
{
	ScopedLock _(&upcall_mtx_);
	if (kind >= SOCKET_UINET_UPCALLS)
		HALT(log_) << "Scheduling invalid upcall.";
	if (upcall_pending_[kind]) {
		ASSERT_NON_NULL(log_, upcall_action_);
		return;
	}
	upcall_pending_[kind] = true;

	/*
	 * See if we can handle this upcall now,
	 * and any others already pending.
	 */
	bool need_recall = upcall_do();

	if (upcall_action_ != NULL) {
		/*
		 * If we don't need recall, then we can
		 * cancel the pending callback, because
		 * we managed to handle the extant
		 * upcalls as well as our own.
		 *
		 * If we do need to recall, we can wait
		 * for the pending callback.
		 */
		if (!need_recall) {
			upcall_action_->cancel();
			upcall_action_ = NULL;

			DEBUG(log_) << "Piggybacked pending upcalls; cancelled callback.";
		}
		return;
	}

	/*
	 * We need recall and there is no pending upcall callback.
	 */
	SimpleCallback *cb = callback(scheduler_, &upcall_mtx_, this, &SocketUinet::upcall_callback);
	upcall_action_ = cb->schedule();
}

void
SocketUinet::upcall_callback(void)
{
	ASSERT_LOCK_OWNED(log_, &upcall_mtx_);
	if (upcall_action_ == NULL) {
		/*
		 * This can happen if a thread coming from libuinet
		 * calls upcall_schedule and then this callback is
		 * dispatched, since the callback scheduler right
		 * now does not itself interlock with our mutexes,
		 * which is the eventual goal.  As a result, that
		 * thread from libuinet can decide to perform
		 * upcall_do() itself, discover that no work is
		 * needed, and then cancel the callback, but the
		 * callback is already being executed here, so we
		 * finish acquiring the lock, and then find that
		 * our action has been cancelled.
		 */
		DEBUG(log_) << "Deferred upcall callback no longer needed.";
		return;
	}
	upcall_action_->cancel();
	upcall_action_ = NULL;

	bool need_recall = upcall_do();
	if (!need_recall)
		return;

	DEBUG(log_) << "Recalling upcalls to avoid deadlock.";

	/*
	 * We could not process every pending upcall, because
	 * we could not acquire the locks using try_lock.  We
	 * must try again, because if we blocked here it could
	 * cause a lock order reversal.
	 */
	SimpleCallback *cb = callback(scheduler_, &upcall_mtx_, this, &SocketUinet::upcall_callback);
	upcall_action_ = cb->schedule();
}

bool
SocketUinet::upcall_do(void)
{
	bool need_recall = false;
	unsigned i;
	for (i = 0; i < SOCKET_UINET_UPCALLS; i++) {
		if (!upcall_pending_[i])
			continue;

		switch (i) {
		case SOCKET_UINET_UPCALL_PASSIVE_RECEIVE:
			if (!accept_connect_mtx_.try_lock()) {
				need_recall = true;
				continue;
			}
			if (accept_callback_ != NULL) {
				accept_schedule();
			} else {
				DEBUG(log_) << "Spurious passive receive upcall.";
			}
			accept_connect_mtx_.unlock();
			break;
		case SOCKET_UINET_UPCALL_ACTIVE_RECEIVE:
			if (!read_mtx_.try_lock()) {
				need_recall = true;
				continue;
			}
			if (read_callback_ != NULL) {
				read_schedule();
			} else {
				DEBUG(log_) << "Spurious active receive upcall.";
			}
			read_mtx_.unlock();
			break;
		case SOCKET_UINET_UPCALL_ACTIVE_SEND:
			if (!write_mtx_.try_lock()) {
				need_recall = true;
				continue;
			}
			if (write_callback_ != NULL) {
				write_schedule();
			} else {
				DEBUG(log_) << "Spurious active send upcall.";
			}
			write_mtx_.unlock();
			break;
		case SOCKET_UINET_UPCALL_CONNECT:
			if (!accept_connect_mtx_.try_lock()) {
				need_recall = true;
				continue;
			}
			if (connect_callback_ != NULL) {
				connect_schedule();
			} else {
				DEBUG(log_) << "Spurious connect upcall.";
			}
			accept_connect_mtx_.unlock();
			break;
		default:
			NOTREACHED(log_);
		}

		upcall_pending_[i] = false;
	}

	return (need_recall);
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

int SocketUinet::gettypenum(SocketType type)
{
	switch (type) {
	case SocketTypeStream: return (UINET_SOCK_STREAM);
#if 0 /* not yet */
	case SocketTypeDatagram: return (UINET_SOCK_DGRAM);
#endif
	default:
		ERROR("/socket/uinet") << "Unsupported socket type.";
		return (-1);
	}
}

int SocketUinet::getprotonum(const std::string& protocol)
{
	if (protocol == "") {
		return(0);
	} else {
		if (protocol == "tcp" || protocol == "TCP") {
			return (UINET_IPPROTO_TCP);
		} else if (protocol == "udp" || protocol == "UDP") {
			return (UINET_IPPROTO_UDP);
		} else {
			ERROR("/socket/uinet") << "Invalid protocol: " << protocol;
			return (-1);
		}
	}
}

int SocketUinet::getdomainnum(SocketAddressFamily family, const std::string& hint)
{
	switch (family) {
#if 0 /* not yet */
	case SocketAddressFamilyIP:
		if (uinet_inet6_enabled()) {
			if (hint == "") {
				ERROR("/socket/uinet") << "Must specify hint address for IP sockets or specify IPv4 or IPv6 explicitly.";
				return (-1);
			} else {
				/ * XXX evaluate hint */
				socket_address addr;

				if (!addr(UINET_AF_UNSPEC, typenum, protonum, hint)) {
					ERROR("/socket/uinet") << "Invalid hint: " << hint;
					return (-1);
				}

				/* XXX Just make socket_address::operator() smarter about AF_UNSPEC?  */
				switch (addr.addr_.sockaddr_.sa_family) {
				case AF_INET: return (UINET_AF_INET);
				case AF_INET6: return(UINET_AF_INET6);
				default:
					ERROR("/socket/uinet") << "Unsupported address family for hint: " << hint;
					return (-1);
				}
				break;
			}
		} else {
			(void)hint;
			return (UINET_AF_INET);
		}
		break;
#else
		(void)hint;
#endif

	case SocketAddressFamilyIPv4:
		return(UINET_AF_INET);
		break;

	case SocketAddressFamilyIPv6:
		if (uinet_inet6_enabled()) {
			return (UINET_AF_INET6);
		} else {
			ERROR("/socket/uinet") << "Unsupported address family.";
			return (-1);
		}
		break;

	default:
		ERROR("/socket/uinet") << "Unsupported address family.";
		return (-1);
	}
}

SocketUinet *
SocketUinet::create(SocketAddressFamily family, SocketType type, const std::string& protocol, const std::string& hint)
{
	return (create_basic<SocketUinet>(family, type, protocol, hint));
}
