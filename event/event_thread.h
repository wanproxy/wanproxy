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

#ifndef	EVENT_EVENT_THREAD_H
#define	EVENT_EVENT_THREAD_H

#include <common/thread/thread.h>
#include <event/callback_queue.h>
#include <event/timeout_queue.h>

enum EventInterest {
	EventInterestReload,
	EventInterestStop
};

class EventThread : public WorkerThread {
	LogHandle log_;
	CallbackQueue queue_;
	bool reload_;
	std::map<EventInterest, CallbackQueue *> interest_queue_;
	TimeoutQueue timeout_queue_;
public:
	EventThread(void);

	~EventThread()
	{ }

public:
	Action *register_interest(const EventInterest& interest, SimpleCallback *cb)
	{
		CallbackQueue *cbq = interest_queue_[interest];
		if (cbq == NULL) {
			cbq = new CallbackQueue();
			interest_queue_[interest] = cbq;
		}
		Action *a = cbq->schedule(cb);
		return (a);
	}

	Action *schedule(CallbackBase *cb)
	{
		Action *a = queue_.schedule(cb);
		submit(); /* XXX Only do so if queue was empty.  */
		return (a);
	}

	Action *timeout(unsigned secs, SimpleCallback *cb)
	{
		Action *a = timeout_queue_.append(secs, cb);
		submit();
		return (a);
	}

private:
	void work(void);
	void wait(void);

public:
	void reload(void);

	static EventThread *self(void)
	{
		Thread *td = Thread::self();
		return (dynamic_cast<EventThread *>(td));
	}
};

#endif /* !EVENT_EVENT_THREAD_H */
