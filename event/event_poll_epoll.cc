/*
 * Copyright (c) 2008-2014 Juli Mallett. All rights reserved.
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

#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_poll.h>

#define	SIGNAL_IDENT	(0x5c0276ef) /* A random number. */

#define	EPOLL_EVENT_COUNT	128

struct EventPollState {
	int ep_;
	int fd_;
};

EventPoll::EventPoll(void)
: Thread("EventPoll"),
  log_("/event/poll"),
  mtx_("EventPoll"),
  read_poll_(),
  write_poll_(),
  state_(new EventPollState())
{
	state_->ep_ = epoll_create(EPOLL_EVENT_COUNT);
	ASSERT(log_, state_->ep_ != -1);

	state_->fd_ = eventfd(0, 0);
	ASSERT(log_, state_->fd_ != -1);

	struct epoll_event eev;
	eev.data.fd = state_->fd_;
	eev.events = EPOLLIN;
	int rv = ::epoll_ctl(state_->ep_, EPOLL_CTL_ADD, state_->fd_, &eev);
	if (rv == -1)
		HALT(log_) << "Could not add eventfd to epoll.";
}

EventPoll::~EventPoll()
{
	ASSERT(log_, read_poll_.empty());
	ASSERT(log_, write_poll_.empty());

	if (state_ != NULL) {
		if (state_->ep_ != -1) {
			close(state_->ep_);
			state_->ep_ = -1;
		}
		if (state_->fd_ != -1) {
			close(state_->fd_);
			state_->fd_ = -1;
		}
		delete state_;
		state_ = NULL;
	}
}

Action *
EventPoll::poll(const Type& type, int fd, EventCallback *cb)
{
	/*
	 * XXX
	 * A wakeup isn't necessary because kqueue provides synchronization, right?
	 * It *is* necessary in the stop() case.
	 */

	ScopedLock _(&mtx_);

	ASSERT(log_, fd != -1);

	EventPoll::PollHandler *poll_handler;
	struct epoll_event eev;
	bool unique = true;
	eev.data.fd = fd;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(log_, read_poll_.find(fd) == read_poll_.end());
		unique = write_poll_.find(fd) == write_poll_.end();
		poll_handler = &read_poll_[fd];
		eev.events = EPOLLIN;
		if (!unique)
			eev.events |= EPOLLOUT;
		break;
	case EventPoll::Writable:
		ASSERT(log_, write_poll_.find(fd) == write_poll_.end());
		unique = read_poll_.find(fd) == read_poll_.end();
		poll_handler = &write_poll_[fd];
		eev.events = EPOLLOUT;
		if (!unique)
			eev.events |= EPOLLIN;
		break;
	default:
		NOTREACHED(log_);
	}
	int rv = ::epoll_ctl(state_->ep_, unique ? EPOLL_CTL_ADD : EPOLL_CTL_MOD, fd, &eev);
	if (rv == -1)
		HALT(log_) << "Could not add event to epoll.";
	ASSERT(log_, rv == 0);
	ASSERT(log_, poll_handler->action_ == NULL);
	poll_handler->callback_ = cb;
	Action *a = new EventPoll::PollAction(this, type, fd);
	return (a);
}

void
EventPoll::cancel(const Type& type, int fd)
{
	ScopedLock _(&mtx_);

	EventPoll::PollHandler *poll_handler;

	struct epoll_event eev;
	bool unique = true;
	eev.data.fd = fd;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(log_, read_poll_.find(fd) != read_poll_.end());
		unique = write_poll_.find(fd) == write_poll_.end();
		poll_handler = &read_poll_[fd];
		poll_handler->cancel();
		read_poll_.erase(fd);
		if (unique)
			eev.events = 0;
		else
			eev.events = EPOLLOUT;
		break;
	case EventPoll::Writable:
		ASSERT(log_, write_poll_.find(fd) != write_poll_.end());
		unique = read_poll_.find(fd) == read_poll_.end();
		poll_handler = &write_poll_[fd];
		poll_handler->cancel();
		write_poll_.erase(fd);
		if (unique)
			eev.events = 0;
		else
			eev.events = EPOLLIN;
		break;
	default:
		NOTREACHED(log_);
	}
	int rv = ::epoll_ctl(state_->ep_, unique ? EPOLL_CTL_DEL : EPOLL_CTL_MOD, fd, &eev);
	if (rv == -1)
		HALT(log_) << "Could not delete event from epoll.";
	ASSERT(log_, rv == 0);
}

void
EventPoll::main(void)
{
	struct epoll_event eev[EPOLL_EVENT_COUNT];

	for (;;) {
		int evcnt = ::epoll_wait(state_->ep_, eev, EPOLL_EVENT_COUNT, -1);
		if (evcnt == -1) {
			if (errno == EINTR) {
				if (stop_) {
					INFO(log_) << "Received interrupt, ceasing polling until stop handlers have run.";
					return;
				}
				continue;
			}
			HALT(log_) << "Could not poll epoll.";
		}

		/*
		 * NB: We could acquire and drop the lock for each item in the
		 * loop, but isn't this more humane?
		 */
		ScopedLock _(&mtx_);
		int i;
		for (i = 0; i < evcnt; i++) {
			struct epoll_event *ev = &eev[i];
			EventPoll::PollHandler *poll_handler;
			poll_handler_map_t::iterator it;

			if (ev->data.fd == state_->fd_) {
				ASSERT(log_, (ev->events & EPOLLIN) != 0);
				/* A user event was triggered to wake us up.  Clear it.  */
				uint64_t cnt;
				ssize_t len = read(state_->fd_, &cnt, sizeof cnt);
				if (len != sizeof cnt)
					ERROR(log_) << "Short read from eventfd.";
				DEBUG(log_) << "Got " << cnt << " wakeup requests.";
				continue;
			}

			if ((it = read_poll_.find(ev->data.fd)) != read_poll_.end()) {
				poll_handler = &it->second;

				if ((ev->events & EPOLLIN) != 0) {
					poll_handler->callback(Event::Done);
				} else if ((ev->events & EPOLLERR) != 0) {
					poll_handler->callback(Event::Error);
				} else if ((ev->events & EPOLLHUP) != 0) {
					poll_handler->callback(Event::EOS);
				}
			}

			if ((it = write_poll_.find(ev->data.fd)) != write_poll_.end()) {
				poll_handler = &it->second;

				if ((ev->events & EPOLLOUT) != 0) {
					poll_handler->callback(Event::Done);
				} else if ((ev->events & EPOLLERR) != 0) {
					poll_handler->callback(Event::Error);
				}
			}
		}

		if (stop_)
			break;
	}
}

void
EventPoll::stop(void)
{
	ScopedLock _(&mtx_);
	if (stop_)
		return;
	uint64_t cnt = 1;
	ssize_t len = write(state_->fd_, &cnt, sizeof cnt);
	if (len != sizeof cnt)
		HALT(log_) << "Short write to eventfd.";

	stop_ = true;
}
