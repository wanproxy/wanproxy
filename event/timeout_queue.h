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

#ifndef	EVENT_TIMEOUT_QUEUE_H
#define	EVENT_TIMEOUT_QUEUE_H

#include <map>

#include <common/thread/mutex.h>
#include <common/time/time.h>

class TimeoutQueue {
	typedef std::map<NanoTime, CallbackQueue *> timeout_map_t;

	LogHandle log_;
	Mutex mtx_;
	timeout_map_t timeout_queue_;
public:
	TimeoutQueue(void)
	: log_("/event/timeout/queue"),
	  mtx_("TimeoutQueue"),
	  timeout_queue_()
	{ }

	~TimeoutQueue()
	{
		ScopedLock _(&mtx_);
		timeout_map_t::iterator it;

		for (it = timeout_queue_.begin(); it != timeout_queue_.end(); ++it)
			delete it->second;
		timeout_queue_.clear();
	}

	/* XXX Can't be const because CallbackQueue::empty can't be due to locking.  */
	bool empty(void)
	{
		ScopedLock _(&mtx_);
		timeout_map_t::iterator it;

		/*
		 * Since we allow elements within each CallbackQueue to be
		 * cancelled, we must scan them.
		 *
		 * XXX
		 * We really shouldn't allow this, even if it means we have
		 * to specialize CallbackQueue for this purpose or add
		 * virtual methods to it.  As it is, we can return true
		 * for empty and for ready at the same time.  And in those
		 * cases we have to call perform to garbage collect the
		 * unused CallbackQueues.  We'll, quite conveniently,
		 * never make that call.  Yikes.
		 */
		for (it = timeout_queue_.begin(); it != timeout_queue_.end(); ++it) {
			if (it->second->empty())
				continue;
			return (false);
		}
		return (true);
	}

	/* Like ::empty() and ::ready(), has to possibly ignore empty queues.  */
	NanoTime next_deadline(void)
	{
		ScopedLock _(&mtx_);
		timeout_map_t::iterator it;

		ASSERT(log_, !timeout_queue_.empty());
		for (it = timeout_queue_.begin(); it != timeout_queue_.end(); ++it) {
			if (it->second->empty())
				continue;
			return (it->first);
		}
		/* XXX Almost NOTREACHED */
		return (NanoTime());
	}

	Action *append(uintmax_t, SimpleCallback *);
	void perform(void);
	bool ready(void);
};

#endif /* !EVENT_TIMEOUT_QUEUE_H */
