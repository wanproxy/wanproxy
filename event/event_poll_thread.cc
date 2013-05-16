/*
 * Copyright (c) 2013 Juli Mallett. All rights reserved.
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
#include <event/event_poll_thread.h>
#include <event/event_system.h>

/* XXX Add an FD to read and write to for signal purposes.  */

EventPollThread::EventPollThread(void)
: Thread("EventPollThread"),
  log_("/event/poll/thread"),
  poll_()
{ }

EventPollThread::~EventPollThread()
{ }

Action *
EventPollThread::poll(const EventPoll::Type& type, int fd, EventCallback *cb)
{
	ScopedLock _(&mtx_);
	Action *a = poll_.poll(type, fd, cb);
	submit();
	return (a);
}

void
EventPollThread::work(void)
{
	ScopedLock _(&mtx_);
	poll_.poll();
}

void
EventPollThread::wait(void)
{
	ScopedLock _(&mtx_);
	poll_.wait();
}

void
EventPollThread::signal(bool stop)
{
	ScopedLock _(&mtx_);
	if (!stop) {
		if (pending_)
			return;
		pending_ = true;
	} else {
		if (stop_)
			return;
		stop_ = true;
	}
}
