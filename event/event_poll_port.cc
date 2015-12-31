/*
 * Copyright (c) 2009-2013 Juli Mallett. All rights reserved.
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
#include <sys/time.h>
#include <errno.h>
#include <poll.h>
#include <port.h>
#include <unistd.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>

/* XXX See state_ in event_poll_kqueue.c */

EventPoll::EventPoll(void)
: log_("/event/poll"),
  read_poll_(),
  write_poll_(),
  port_(port_create())
{
	ASSERT(log_, port_ != -1);
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
	bool dissociate = false;
	int events;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(log_, read_poll_.find(fd) == read_poll_.end());
		dissociate = write_poll_.find(fd) != write_poll_.end();
		poll_handler = &read_poll_[fd];
		events = POLLIN | (dissociate ? POLLOUT : 0);
		break;
	case EventPoll::Writable:
		ASSERT(log_, write_poll_.find(fd) == write_poll_.end());
		dissociate = read_poll_.find(fd) != read_poll_.end();
		poll_handler = &write_poll_[fd];
		events = POLLOUT | (dissociate ? POLLIN : 0);
		break;
	default:
		NOTREACHED(log_);
	}
	if (dissociate) {
		int rv = ::port_dissociate(port_, PORT_SOURCE_FD, fd);
		if (rv == -1)
			HALT(log_) << "Could not dissociate from port.";
	}
	ASSERT_NON_ZERO(log_, events);
	int rv = ::port_associate(port_, PORT_SOURCE_FD, fd, events, NULL);
	if (rv == -1)
		HALT(log_) << "Could not associate to port.";
	ASSERT_ZERO(log_, rv);
	ASSERT_NULL(log_, poll_handler->action_);
	poll_handler->callback_ = cb;
	Action *a = new EventPoll::PollAction(this, type, fd);
	return (a);
}

void
EventPoll::cancel(const Type& type, int fd)
{
	EventPoll::PollHandler *poll_handler;

	bool associate = false;
	int events = 0;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(log_, read_poll_.find(fd) != read_poll_.end());
		poll_handler = &read_poll_[fd];
		poll_handler->cancel();
		read_poll_.erase(fd);

		if (write_poll_.find(fd) != write_poll_.end()) {
			associate = true;
			events = POLLOUT;
		}
		break;
	case EventPoll::Writable:
		ASSERT(log_, write_poll_.find(fd) != write_poll_.end());
		poll_handler = &write_poll_[fd];
		poll_handler->cancel();
		write_poll_.erase(fd);

		if (read_poll_.find(fd) != read_poll_.end()) {
			associate = true;
			events = POLLIN;
		}
		break;
	}
	int rv = ::port_dissociate(port_, PORT_SOURCE_FD, fd);
	if (rv == -1)
		HALT(log_) << "Could not disassociate from port.";
	ASSERT_ZERO(log_, rv);
	if (associate) {
		ASSERT_NON_ZERO(log_, events);
		rv = ::port_associate(port_, PORT_SOURCE_FD, fd, events, NULL);
		if (rv == -1)
			HALT(log_) << "Could not associate to port.";
	}
}

void
EventPoll::wait(int ms)
{
	static const unsigned pevcnt = 128;
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

	port_event_t pev[pevcnt];
	unsigned evcnt = 1;
	int rv = port_getn(port_, pev, pevcnt, &evcnt, ms == -1 ? NULL : &ts);
	if (rv == -1) {
		if (errno == EINTR) {
			INFO(log_) << "Received interrupt, ceasing polling until stop handlers have run.";
			return;
		}
		HALT(log_) << "Could not poll port.";
	}

	unsigned i;
	for (i = 0; i < evcnt; i++) {
		port_event_t *ev = &pev[i];
		int fd = ev->portev_object;

		/*
		 * The port(3C) mechanism is one-shot, unfortunately, so we
		 * need to re-arm here.  Actually, we're one shot, too, and
		 * so we basically only need to re-arm:
		 * 	events & ~ev->portev_events
		 * but since we don't want to make cancel() processing too
		 * hardcore, we'll settle for this for now.
		 */
		int events = 0;
		if (read_poll_.find(fd) != read_poll_.end())
			events |= POLLIN;
		if (write_poll_.find(fd) != write_poll_.end())
			events |= POLLOUT;
		rv = ::port_associate(port_, PORT_SOURCE_FD, fd, events, NULL);
		if (rv == -1) {
			HALT(log_) << "Could not associate to port.";
		}

		EventPoll::PollHandler *poll_handler;
		if ((ev->portev_events & POLLIN) != 0) {
			ASSERT(log_, read_poll_.find(fd) != read_poll_.end());
			poll_handler = &read_poll_[fd];
			poll_handler->callback(Event::Done);
		}

		if ((ev->portev_events & POLLOUT) != 0) {
			ASSERT(log_, write_poll_.find(fd) != write_poll_.end());
			poll_handler = &write_poll_[fd];
			poll_handler->callback(Event::Done);
		}

		if ((ev->portev_events & POLLIN) == 0 && (ev->portev_events & POLLOUT) == 0) {
			if (read_poll_.find(fd) != read_poll_.end()) {
				poll_handler = &read_poll_[fd];
			} else if (write_poll_.find(fd) != write_poll_.end()) {
				poll_handler = &write_poll_[fd];
			} else {
				HALT(log_) << "Unexpected poll fd.";
				continue;
			}

			if ((ev->portev_events & POLLERR) != 0) {
				poll_handler->callback(Event::Error);
				continue;
			}

			if ((ev->portev_events & POLLHUP) != 0) {
				/* XXX Read only!  */
				poll_handler->callback(Event::EOS);
				continue;
			}

			HALT(log_) << "Unexpected poll events: " << ev->portev_events;
		}
	}
}
