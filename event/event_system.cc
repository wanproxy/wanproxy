/*
 * Copyright (c) 2008-2016 Juli Mallett. All rights reserved.
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
#include <event/event_system.h>

EventSystem::EventSystem(void)
: td_("EventThread"),
  poll_(),
  timeout_(),
  destroy_(),
  threads_(),
  interest_queue_mtx_("EventSystem::interest_queue"),
  interest_queue_()
{ }

Action *
EventSystem::register_interest(const EventInterest& interest, SimpleCallback *cb)
{
	ScopedLock _(&interest_queue_mtx_);
	CallbackQueue *cbq = interest_queue_[interest];
	if (cbq == NULL) {
		cbq = new CallbackQueue();
		interest_queue_[interest] = cbq;
	}
	Action *a = cbq->schedule(cb);
	return (a);
}

void
EventSystem::stop(void)
{
	/*
	 * If we have been told to stop, fire all shutdown events.
	 */
	interest_queue_mtx_.lock();
	CallbackQueue *q = interest_queue_[EventInterestStop];
	if (q != NULL)
		interest_queue_.erase(EventInterestStop);
	interest_queue_mtx_.unlock();
	if (q != NULL && !q->empty()) {
		INFO("/event/system") << "Queueing stop handlers.";
		q->drain();
	}

	/*
	 * Pass stop notification on to all threads.
	 */
	std::deque<Thread *>::const_iterator it;
	for (it = threads_.begin(); it != threads_.end(); ++it) {
		Thread *td = *it;
		td->stop();
	}
}
