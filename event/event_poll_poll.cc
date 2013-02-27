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

#include <sys/errno.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

#include <common/buffer.h>

#include <event/event_callback.h>
#include <event/event_poll.h>

EventPoll::EventPoll(void)
: log_("/event/poll"),
  read_poll_(),
  write_poll_(),
  state_(NULL)
{
}

EventPoll::~EventPoll()
{
	ASSERT(log_, read_poll_.empty());
	ASSERT(log_, write_poll_.empty());
}

Action *
EventPoll::poll(const Type& type, int fd, EventCallback *cb)
{
	ASSERT(log_, fd != -1);

	EventPoll::PollHandler *poll_handler;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(log_, read_poll_.find(fd) == read_poll_.end());
		poll_handler = &read_poll_[fd];
		break;
	case EventPoll::Writable:
		ASSERT(log_, write_poll_.find(fd) == write_poll_.end());
		poll_handler = &write_poll_[fd];
		break;
	default:
		NOTREACHED(log_);
	}
	ASSERT(log_, poll_handler->action_ == NULL);
	poll_handler->callback_ = cb;
	Action *a = new EventPoll::PollAction(this, type, fd);
	return (a);
}

void
EventPoll::cancel(const Type& type, int fd)
{
	EventPoll::PollHandler *poll_handler;

	switch (type) {
	case EventPoll::Readable:
		ASSERT(log_, read_poll_.find(fd) != read_poll_.end());
		poll_handler = &read_poll_[fd];
		poll_handler->cancel();
		read_poll_.erase(fd);
		break;
	case EventPoll::Writable:
		ASSERT(log_, write_poll_.find(fd) != write_poll_.end());
		poll_handler = &write_poll_[fd];
		poll_handler->cancel();
		write_poll_.erase(fd);
		break;
	}
}

void
EventPoll::wait(int ms)
{
	poll_handler_map_t::iterator it;
	std::map<int, short> event_map;

	for (it = read_poll_.begin(); it != read_poll_.end(); ++it)
		event_map[it->first] |= POLLIN;

	for (it = write_poll_.begin(); it != write_poll_.end(); ++it)
		event_map[it->first] |= POLLOUT;

	size_t nfds = event_map.size();
	struct pollfd fds[nfds];
	unsigned i = 0;

	std::map<int, short>::iterator eit;
	for (eit = event_map.begin(); eit != event_map.end(); ++eit) {
		struct pollfd *fd = &fds[i++];
		fd->fd = eit->first;
		fd->events = eit->second;
		fd->revents = 0;
	}

	/*
	 * XXX Why wait to call idle() until here?
	 */
	if (idle()) {
		if (ms != -1) {
			int rv;

			rv = usleep(ms * 1000);
			ASSERT(log_, rv != -1);
		}
		return;
	}

	int fdcnt = ::poll(fds, nfds, ms);
	if (fdcnt == -1) {
		if (errno == EINTR) {
			INFO(log_) << "Received interrupt, ceasing polling until stop handlers have run.";
			return;
		}
		HALT(log_) << "Could not poll.";
	}

	for (i = 0; i < nfds; i++) {
		struct pollfd *fd = &fds[i];

		if (fd->revents == 0)
			continue;

		if (nfds-- == 0)
			break;

		EventPoll::PollHandler *poll_handler;
		if ((fd->revents & POLLIN) != 0) {
			ASSERT(log_, read_poll_.find(fd->fd) != read_poll_.end());
			poll_handler = &read_poll_[fd->fd];
		} else if ((fd->revents & POLLOUT) != 0) {
			ASSERT(log_, write_poll_.find(fd->fd) != write_poll_.end());
			poll_handler = &write_poll_[fd->fd];
		} else {
			if ((fd->events & POLLIN) != 0) {
				ASSERT(log_, read_poll_.find(fd->fd) !=
				       read_poll_.end());
				poll_handler = &read_poll_[fd->fd];
			} else if ((fd->events & POLLOUT) != 0) {
				ASSERT(log_, write_poll_.find(fd->fd) !=
				       write_poll_.end());
				poll_handler = &write_poll_[fd->fd];
			} else {
				HALT(log_) << "Unexpected poll fd.";
				continue;
			}
		}
		if ((fd->revents & POLLERR) != 0 ||
		    (fd->revents & POLLNVAL) != 0) {
			poll_handler->callback(Event::Error);
			continue;
		}
		if ((fd->revents & POLLHUP) != 0) {
			poll_handler->callback(Event::EOS);
			continue;
		}
		poll_handler->callback(Event::Done);
	}
}
