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
#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>

#include <common/limits.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/io_system.h>

#define	IO_READ_BUFFER_SIZE	65536

IOSystem::Handle::Handle(CallbackScheduler *scheduler, int fd, Channel *owner)
: log_("/io/system/handle"),
  mtx_("IOSystem::Handle"),
  scheduler_(scheduler),
  fd_(fd),
  owner_(owner),
  read_offset_(-1),
  read_amount_(0),
  read_buffer_(),
  read_callback_(NULL),
  read_action_(NULL),
  write_offset_(-1),
  write_buffer_(),
  write_callback_(NULL),
  write_action_(NULL)
{ }

IOSystem::Handle::~Handle()
{
	ASSERT(log_, fd_ == -1);

	ASSERT(log_, read_action_ == NULL);
	ASSERT(log_, read_callback_ == NULL);

	ASSERT(log_, write_action_ == NULL);
	ASSERT(log_, write_callback_ == NULL);
}

Action *
IOSystem::Handle::close_do(SimpleCallback *cb)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);

	ASSERT(log_, fd_ != -1);
	int rv = ::close(fd_);
	if (rv == -1) {
		/*
		 * We display the error here but do not pass it to the
		 * upper layers because it is almost impossible to get
		 * handling of close failing correct.
		 *
		 * For most errors, close fails because it already was
		 * closed by a peer or something like that, so it's as
		 * good as close succeeding.  Only EINTR is allowed to
		 * leave the file descriptor in an undefined state, in
		 * POSIX, although most actual kernels are kinder than
		 * that, and specify that descriptors are closed then.
		 */
		ERROR(log_) << "Close returned error: " << strerror(errno);
	}
	fd_ = -1;
	return (cb->schedule());
}

void
IOSystem::Handle::read_callback(Event e)
{
	ScopedLock _(&mtx_);
	read_action_->cancel();
	read_action_ = NULL;

	switch (e.type_) {
	case Event::EOS:
	case Event::Done:
		break;
	case Event::Error: {
		DEBUG(log_) << "Poll returned error: " << e;
		read_callback_->param(e);
		Action *a = read_callback_->schedule();
		read_action_ = a;
		read_callback_ = NULL;
		return;
	}
	default:
		HALT(log_) << "Unexpected event: " << e;
	}

	read_action_ = read_do();
	if (read_action_ == NULL)
		read_action_ = read_schedule();
	ASSERT(log_, read_action_ != NULL);
}

void
IOSystem::Handle::read_cancel(void)
{
	ScopedLock _(&mtx_);
	ASSERT(log_, read_action_ != NULL);
	read_action_->cancel();
	read_action_ = NULL;

	if (read_callback_ != NULL) {
		delete read_callback_;
		read_callback_ = NULL;
	}
}

Action *
IOSystem::Handle::read_do(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	ASSERT(log_, read_action_ == NULL);

	if (!read_buffer_.empty() && read_buffer_.length() >= read_amount_) {
		if (read_amount_ == 0)
			read_amount_ = read_buffer_.length();
		read_callback_->param(Event(Event::Done, Buffer(read_buffer_, read_amount_)));
		Action *a = read_callback_->schedule();
		read_callback_ = NULL;
		read_buffer_.skip(read_amount_);
		read_amount_ = 0;
		return (a);
	}

	/*
	 * A bit of discussion is warranted on this:
	 *
	 * In tack, IOV_MAX BufferSegments are allocated and read in to
	 * with readv(2), and then the lengths are adjusted and the ones
	 * that are empty are freed.  It's also possible to set the
	 * expected lengths first (and only allocate
	 * 	roundup(rlen, BUFFER_SEGMENT_SIZE) / BUFFER_SEGMENT_SIZE
	 * BufferSegments, though really IOV_MAX (or some chosen number)
	 * seems a bit better since most of our reads right now are
	 * read_amount_==0) and put them into a Buffer and trim the
	 * leftovers, which is a bit nicer.
	 *
	 * Since our read_amount_ is usually 0, though, we're kind of at
	 * the mercy of chance (well, not really) as to how much data we
	 * will read, which means a sizable amount of thrashing of memory;
	 * allocating and freeing BufferSegments.
	 *
	 * By comparison, stack space is cheap in userland and allocating
	 * 64K of it here is pretty painless.  Reading to it is fast and
	 * then copying only what we need into BufferSegments isn't very
	 * costly.  Indeed, since readv can't sparsely-populate each data
	 * pointer, it has to do some data shuffling, already.
	 *
	 * Benchmarking used to show that readv was actually markedly
	 * slower here, primarily because of the need to new and delete
	 * lots of BufferSegments.  Now that there is a BufferSegment
	 * cache, that cost is significantly lowered.  It is probably a
	 * good idea to reevaluate it now, especially if we can stomach
	 * also keeping a small cache of BufferSegments just for this
	 * IOSystem Handle.
	 */
	uint8_t data[IO_READ_BUFFER_SIZE];
	ssize_t len;
	if (read_offset_ == -1) {
		len = ::read(fd_, data, sizeof data);
	} else {
		/*
		 * For offset reads, we do not read extra data since we do
		 * not know whether the next read will be to the subsequent
		 * location.
		 *
		 * This makes even more sense since we don't allow 0-length
		 * offset reads.
		 */
		size_t size = std::min(sizeof data, read_amount_);
		len = ::pread(fd_, data, size, read_offset_);
		if (len > 0)
			read_offset_ += len;
	}
	if (len == -1) {
		switch (errno) {
		case EAGAIN:
			return (NULL);
		default:
			read_callback_->param(Event(Event::Error, errno, read_buffer_));
			Action *a = read_callback_->schedule();
			read_callback_ = NULL;
			read_buffer_.clear();
			read_amount_ = 0;
			return (a);
		}
		NOTREACHED(log_);
	}

	/*
	 * XXX
	 * If we get a short read from readv and detected EOS from
	 * EventPoll is that good enough, instead?  We can keep reading
	 * until we get a 0, sure, but if things other than network
	 * conditions influence whether reads would block (and whether
	 * non-blocking reads return), there could be more data waiting,
	 * and so we shouldn't just use a short read as an indicator?
	 */
	if (len == 0) {
		read_callback_->param(Event(Event::EOS, read_buffer_));
		Action *a = read_callback_->schedule();
		read_callback_ = NULL;
		read_buffer_.clear();
		read_amount_ = 0;
		return (a);
	}

	read_buffer_.append(data, len);

	if (!read_buffer_.empty() &&
	    read_buffer_.length() >= read_amount_) {
		if (read_amount_ == 0)
			read_amount_ = read_buffer_.length();
		read_callback_->param(Event(Event::Done, Buffer(read_buffer_, read_amount_)));
		Action *a = read_callback_->schedule();
		read_callback_ = NULL;
		read_buffer_.skip(read_amount_);
		read_amount_ = 0;
		return (a);
	}

	/*
	 * We may do another read without polling, but yield to other
	 * callbacks.
	 */
	if (len == sizeof data) {
		/* TODO */
		DEBUG(log_) << "Could read without polling.";
	}

	return (NULL);
}

Action *
IOSystem::Handle::read_schedule(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	ASSERT(log_, read_action_ == NULL);

	EventCallback *cb = callback(scheduler_, this, &IOSystem::Handle::read_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Readable, fd_, cb);
	return (a);
}

void
IOSystem::Handle::write_callback(Event e)
{
	ScopedLock _(&mtx_);
	write_action_->cancel();
	write_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::Error: {
		DEBUG(log_) << "Poll returned error: " << e;
		write_callback_->param(e);
		Action *a = write_callback_->schedule();
		write_action_ = a;
		write_callback_ = NULL;
		return;
	}
	default:
		HALT(log_) << "Unexpected event: " << e;
	}

	write_action_ = write_do();
	if (write_action_ == NULL)
		write_action_ = write_schedule();
	ASSERT(log_, write_action_ != NULL);
}

void
IOSystem::Handle::write_cancel(void)
{
	ScopedLock _(&mtx_);
	ASSERT(log_, write_action_ != NULL);
	write_action_->cancel();
	write_action_ = NULL;

	if (write_callback_ != NULL) {
		delete write_callback_;
		write_callback_ = NULL;
	}
}

Action *
IOSystem::Handle::write_do(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);

	/*
	 * XXX
	 *
	 * This doesn't handle UDP nicely.  Right?
	 *
	 * If a UDP packet is > IOV_MAX segments, this will break it.
	 * Need something like mbuf(9)'s m_collapse(), where we can demand
	 * that the Buffer fit into IOV_MAX segments, rather than saying
	 * that we want the first IOV_MAX segments.  Easy enough to combine
	 * the unshared BufferSegments?
	 */
	struct iovec iov[IOV_MAX];
	size_t iovcnt = write_buffer_.fill_iovec(iov, IOV_MAX);
	ASSERT(log_, iovcnt != 0);

	ssize_t len;
	if (write_offset_ == -1) {
		len = ::writev(fd_, iov, iovcnt);
	} else {
#if defined(__FreeBSD__)
		len = ::pwritev(fd_, iov, iovcnt, write_offset_);
		if (len > 0)
			write_offset_ += len;
#else
		/*
		 * XXX
		 * Thread unsafe.
		 */
		off_t off = lseek(fd_, write_offset_, SEEK_SET);
		if (off == -1) {
			len = -1;
		} else {
			len = ::writev(fd_, iov, iovcnt);
			if (len > 0)
				write_offset_ += len;
		}

		/*
		 * XXX
		 * Slow!
		 */
#if 0
		unsigned i;

		if (iovcnt == 0) {
			len = -1;
			errno = EINVAL;
		}

		for (i = 0; i < iovcnt; i++) {
			struct iovec *iovp = &iov[i];

			ASSERT(log_, iovp->iov_len != 0);

			len = ::pwrite(fd_, iovp->iov_base, iovp->iov_len,
				       write_offset_);
			if (len <= 0)
				break;

			write_offset_ += len;

			/*
			 * Partial write.
			 */
			if ((size_t)len != iovp->iov_len)
				break;
		}
#endif
#endif
	}
	if (len == -1) {
		switch (errno) {
		case EAGAIN:
			return (NULL);
		default:
			write_callback_->param(Event(Event::Error, errno));
			Action *a = write_callback_->schedule();
			write_callback_ = NULL;
			return (a);
		}
		NOTREACHED(log_);
	}

	write_buffer_.skip(len);

	if (write_buffer_.empty()) {
		write_callback_->param(Event::Done);
		Action *a = write_callback_->schedule();
		write_callback_ = NULL;
		return (a);
	}
	return (NULL);
}

Action *
IOSystem::Handle::write_schedule(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	ASSERT(log_, write_action_ == NULL);

	EventCallback *cb = callback(scheduler_, this, &IOSystem::Handle::write_callback);
	Action *a = EventSystem::instance()->poll(EventPoll::Writable, fd_, cb);
	return (a);
}
