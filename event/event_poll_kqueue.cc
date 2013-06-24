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

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_poll.h>

#define	SIGNAL_IDENT	(0x5c0276ef) /* A random number. */

struct EventPollState {
	int kq_;
};

EventPoll::EventPoll(void)
: Thread("EventPoll"),
  log_("/event/poll"),
  mtx_("EventPoll"),
  read_poll_(),
  write_poll_(),
  state_(new EventPollState())
{
	state_->kq_ = kqueue();
	ASSERT(log_, state_->kq_ != -1);

	struct kevent kev;
	EV_SET(&kev, SIGNAL_IDENT, EVFILT_USER, EV_ADD, 0, 0, NULL);
	int evcnt = ::kevent(state_->kq_, &kev, 1, NULL, 0, NULL);
	if (evcnt == -1)
		HALT(log_) << "Could not add self-signal event to kqueue.";
	ASSERT(log_, evcnt == 0);
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
	/*
	 * XXX
	 * A wakeup isn't necessary because kqueue provides synchronization, right?
	 * It *is* necessary in the stop() case.
	 */

	ScopedLock _(&mtx_);

	ASSERT(log_, fd != -1);

	EventPoll::PollHandler *poll_handler;
	struct kevent kev;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(log_, read_poll_.find(fd) == read_poll_.end());
		poll_handler = &read_poll_[fd];
		EV_SET(&kev, fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, NULL);
		break;
	case EventPoll::Writable:
		ASSERT(log_, write_poll_.find(fd) == write_poll_.end());
		poll_handler = &write_poll_[fd];
		EV_SET(&kev, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, NULL);
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
	ScopedLock _(&mtx_);

	/*
	 * XXX MT XXX
	 * Needs to delete the evfilter iff unfired.
	 */
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
EventPoll::main(void)
{
	static const unsigned kevcnt = 128;
	struct kevent kev[kevcnt];

	for (;;) {
		int evcnt = kevent(state_->kq_, NULL, 0, kev, kevcnt, NULL);
		if (evcnt == -1) {
			if (errno == EINTR) {
				INFO(log_) << "Received interrupt, ceasing polling until stop handlers have run.";
				return;
			}
			HALT(log_) << "Could not poll kqueue.";
		}

		/*
		 * NB: We could acquire and drop the lock for each item in the
		 * loop, but isn't this more humane?
		 */
		ScopedLock _(&mtx_);
		int i;
		for (i = 0; i < evcnt; i++) {
			struct kevent *ev = &kev[i];
			EventPoll::PollHandler *poll_handler;
			poll_handler_map_t::iterator it;

			switch (ev->filter) {
			case EVFILT_READ:
				it = read_poll_.find(ev->ident);
				if (it == read_poll_.end()) {
					DEBUG(log_) << "Dropping read event lost in race.";
					continue;
				}
				break;
			case EVFILT_WRITE:
				it = write_poll_.find(ev->ident);
				if (it == write_poll_.end()) {
					DEBUG(log_) << "Dropping write event lost in race.";
					continue;
				}
				break;
			case EVFILT_USER:
				/* A user event was triggered to wake us up.  Ignore it.  */
				ASSERT(log_, ev->ident == SIGNAL_IDENT);
				continue;
			default:
				NOTREACHED(log_);
			}
			poll_handler = &it->second;
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
	struct kevent kev;
	EV_SET(&kev, SIGNAL_IDENT, EVFILT_USER, 0, NOTE_TRIGGER, 0, NULL);
	int evcnt = ::kevent(state_->kq_, &kev, 1, NULL, 0, NULL);
	if (evcnt == -1)
		HALT(log_) << "Could not trigger self-signal event in kqueue.";
	ASSERT(log_, evcnt == 0);

	stop_ = true;
}
