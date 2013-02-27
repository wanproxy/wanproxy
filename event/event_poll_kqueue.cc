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

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_poll.h>

struct EventPollState {
	int kq_;
};

EventPoll::EventPoll(void)
: log_("/event/poll"),
  read_poll_(),
  write_poll_(),
  state_(new EventPollState())
{
	state_->kq_ = kqueue();
	ASSERT(log_, state_->kq_ != -1);
}

EventPoll::~EventPoll()
{
	ASSERT(log_, read_poll_.empty());
	ASSERT(log_, write_poll_.empty());

	if (state_ != NULL) {
		if (state_->kq_ != -1) {
			close(state_->kq_);
			state_->kq_ = -1;
		}
		delete state_;
		state_ = NULL;
	}
}

Action *
EventPoll::poll(const Type& type, int fd, EventCallback *cb)
{
	ASSERT(log_, fd != -1);

	EventPoll::PollHandler *poll_handler;
	struct kevent kev;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(log_, read_poll_.find(fd) == read_poll_.end());
		poll_handler = &read_poll_[fd];
		EV_SET(&kev, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
		break;
	case EventPoll::Writable:
		ASSERT(log_, write_poll_.find(fd) == write_poll_.end());
		poll_handler = &write_poll_[fd];
		EV_SET(&kev, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
		break;
	default:
		NOTREACHED(log_);
	}
	int evcnt = ::kevent(state_->kq_, &kev, 1, NULL, 0, NULL);
	if (evcnt == -1)
		HALT(log_) << "Could not add event to kqueue.";
	ASSERT(log_, evcnt == 0);
	ASSERT(log_, poll_handler->action_ == NULL);
	poll_handler->callback_ = cb;
	Action *a = new EventPoll::PollAction(this, type, fd);
	return (a);
}

void
EventPoll::cancel(const Type& type, int fd)
{
	EventPoll::PollHandler *poll_handler;

	struct kevent kev;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(log_, read_poll_.find(fd) != read_poll_.end());
		poll_handler = &read_poll_[fd];
		poll_handler->cancel();
		read_poll_.erase(fd);
		EV_SET(&kev, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		break;
	case EventPoll::Writable:
		ASSERT(log_, write_poll_.find(fd) != write_poll_.end());
		poll_handler = &write_poll_[fd];
		poll_handler->cancel();
		write_poll_.erase(fd);
		EV_SET(&kev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
		break;
	}
	int evcnt = ::kevent(state_->kq_, &kev, 1, NULL, 0, NULL);
	if (evcnt == -1)
		HALT(log_) << "Could not delete event from kqueue.";
	ASSERT(log_, evcnt == 0);
}

void
EventPoll::wait(int ms)
{
	static const unsigned kevcnt = 128;
	struct timespec ts;

	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;

	if (idle()) {
		if (ms != -1) {
			int rv;

			rv = nanosleep(&ts, NULL);
			ASSERT(log_, rv != -1);
		}
		return;
	}

	struct kevent kev[kevcnt];
	int evcnt = kevent(state_->kq_, NULL, 0, kev, kevcnt, ms == -1 ? NULL : &ts);
	if (evcnt == -1) {
		if (errno == EINTR) {
			INFO(log_) << "Received interrupt, ceasing polling until stop handlers have run.";
			return;
		}
		HALT(log_) << "Could not poll kqueue.";
	}

	int i;
	for (i = 0; i < evcnt; i++) {
		struct kevent *ev = &kev[i];
		EventPoll::PollHandler *poll_handler;
		switch (ev->filter) {
		case EVFILT_READ:
			ASSERT(log_, read_poll_.find(ev->ident) != read_poll_.end());
			poll_handler = &read_poll_[ev->ident];
			break;
		case EVFILT_WRITE:
			ASSERT(log_, write_poll_.find(ev->ident) !=
			       write_poll_.end());
			poll_handler = &write_poll_[ev->ident];
			break;
		default:
			NOTREACHED(log_);
		}
		if ((ev->flags & EV_ERROR) != 0) {
			poll_handler->callback(Event(Event::Error, ev->fflags));
			continue;
		}
		if ((ev->flags & EV_EOF) != 0 && ev->filter == EVFILT_READ) {
			poll_handler->callback(Event(Event::EOS, ev->fflags));
			continue;
		}
		/*
		 * XXX
		 * We do not currently have a way to indicate that the reader
		 * has called shutdown and will no longer read data.  We just
		 * indicate Done and let the next write fail.
		 */
		poll_handler->callback(Event(Event::Done, ev->fflags));
	}
}
