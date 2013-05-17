/*
 * Copyright (c) 2008-2011 Juli Mallett. All rights reserved.
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

#include <sys/resource.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>

#include <event/event_callback.h>

#include <io/io_system.h>

IOSystem::IOSystem(void)
: log_("/io/system"),
  handle_map_()
{
	/*
	 * Prepare system to handle IO.
	 */
	INFO(log_) << "Starting IO system.";

	/*
	 * Disable SIGPIPE.
	 *
	 * Because errors are returned asynchronously and may occur at any
	 * time, there may be a pending write to a file descriptor which
	 * has previously thrown an error.  There are changes that could
	 * be made to the scheduler to work around this, but they are not
	 * desirable.
	 */
	if (::signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		HALT(log_) << "Could not disable SIGPIPE.";

	/*
	 * Up the file descriptor limit.
	 *
	 * Probably this should be configurable, but there's no harm on
	 * modern systems and for the performance-critical applications
	 * using the IO system, more file descriptors is better.
	 */
	struct rlimit rlim;
	int rv = ::getrlimit(RLIMIT_NOFILE, &rlim);
	if (rv == 0) {
		if (rlim.rlim_cur < rlim.rlim_max) {
			rlim.rlim_cur = rlim.rlim_max;

			rv = ::setrlimit(RLIMIT_NOFILE, &rlim);
			if (rv == -1) {
				INFO(log_) << "Unable to increase file descriptor limit.";
			}
		}
	} else {
		INFO(log_) << "Unable to get file descriptor limit.";
	}
}

IOSystem::~IOSystem()
{
	ASSERT(log_, handle_map_.empty());
}

void
IOSystem::attach(int fd, Channel *owner)
{
	ASSERT(log_, handle_map_.find(handle_key_t(fd, owner)) == handle_map_.end());
	handle_map_[handle_key_t(fd, owner)] = new IOSystem::Handle(fd, owner);
}

void
IOSystem::detach(int fd, Channel *owner)
{
	handle_map_t::iterator it;
	IOSystem::Handle *h;

	it = handle_map_.find(handle_key_t(fd, owner));
	ASSERT(log_, it != handle_map_.end());

	h = it->second;
	ASSERT(log_, h != NULL);
	ASSERT(log_, h->owner_ == owner);

	handle_map_.erase(it);
	delete h;
}

Action *
IOSystem::close(int fd, Channel *owner, SimpleCallback *cb)
{
	IOSystem::Handle *h;

	h = handle_map_[handle_key_t(fd, owner)];
	ASSERT(log_, h != NULL);

	ASSERT(log_, h->read_callback_ == NULL);
	ASSERT(log_, h->read_action_ == NULL);

	ASSERT(log_, h->write_callback_ == NULL);
	ASSERT(log_, h->write_action_ == NULL);

	ASSERT(log_, h->fd_ != -1);

	return (h->close_do(cb));
}

Action *
IOSystem::read(int fd, Channel *owner, off_t offset, size_t amount, EventCallback *cb)
{
	IOSystem::Handle *h;

	h = handle_map_[handle_key_t(fd, owner)];
	ASSERT(log_, h != NULL);

	ASSERT(log_, h->read_callback_ == NULL);
	ASSERT(log_, h->read_action_ == NULL);

	/*
	 * Reads without an offset may be 0 length, but reads with
	 * an offset must have a specified length.
	 */
	ASSERT(log_, offset == -1 || amount != 0);

	/*
	 * If we have an offset, we must invalidate any outstanding
	 * buffers, since they are for data that may not be relevant
	 * to us.
	 */
	if (offset != -1) {
		h->read_buffer_.clear();
	}

	h->read_offset_ = offset;
	h->read_amount_ = amount;
	h->read_callback_ = cb;
	Action *a = h->read_do();
	ASSERT(log_, h->read_action_ == NULL);
	if (a == NULL) {
		ASSERT(log_, h->read_callback_ != NULL);
		h->read_action_ = h->read_schedule();
		ASSERT(log_, h->read_action_ != NULL);
		return (cancellation(h, &IOSystem::Handle::read_cancel));
	}
	ASSERT(log_, h->read_callback_ == NULL);
	return (a);
}

Action *
IOSystem::write(int fd, Channel *owner, off_t offset, Buffer *buffer, EventCallback *cb)
{
	IOSystem::Handle *h;

	h = handle_map_[handle_key_t(fd, owner)];
	ASSERT(log_, h != NULL);

	ASSERT(log_, h->write_callback_ == NULL);
	ASSERT(log_, h->write_action_ == NULL);
	ASSERT(log_, h->write_buffer_.empty());

	ASSERT(log_, !buffer->empty());
	buffer->moveout(&h->write_buffer_);

	h->write_offset_ = offset;
	h->write_callback_ = cb;
	Action *a = h->write_do();
	ASSERT(log_, h->write_action_ == NULL);
	if (a == NULL) {
		ASSERT(log_, h->write_callback_ != NULL);
		h->write_action_ = h->write_schedule();
		ASSERT(log_, h->write_action_ != NULL);
		return (cancellation(h, &IOSystem::Handle::write_cancel));
	}
	ASSERT(log_, h->write_callback_ == NULL);
	return (a);
}
