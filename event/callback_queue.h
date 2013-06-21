/*
 * Copyright (c) 2010-2013 Juli Mallett. All rights reserved.
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

#ifndef	EVENT_CALLBACK_QUEUE_H
#define	EVENT_CALLBACK_QUEUE_H

#include <deque>

#include <common/thread/mutex.h>

#include <event/callback.h>

class CallbackQueue : public CallbackScheduler {
	class CallbackAction : public Cancellable {
	public:
		CallbackQueue *const queue_;
		CallbackBase *callback_;
		Action *action_;

		CallbackAction(CallbackQueue *queue, CallbackBase *callback)
		: Cancellable(),
		  queue_(queue),
		  callback_(callback),
		  action_(NULL)
		{ }

		~CallbackAction()
		{
			ASSERT("/callback/queue/action", callback_ == NULL);
			ASSERT("/callback/queue/action", action_ == NULL);
		}

		void cancel(void)
		{
			queue_->cancel(this);
		}
	};

	friend class CallbackAction;

	Mutex mtx_;
	std::deque<CallbackAction *> queue_;
public:
	CallbackQueue(void)
	: mtx_("CallbackQueue"),
	  queue_()
	{ }

	~CallbackQueue()
	{
		ASSERT("/callback/queue", queue_.empty());
	}

	Action *schedule(CallbackBase *cb)
	{
		CallbackAction *a = new CallbackAction(this, cb);

		mtx_.lock();
		queue_.push_back(a);
		mtx_.unlock();

		return (a);
	}

	void drain(void)
	{
		ScopedLock _(&mtx_);
		while (!queue_.empty()) {
			CallbackAction *a = queue_.front();
			ASSERT("/callback/queue", a->queue_ == this);
			a->action_ = a->callback_->schedule();
			a->callback_ = NULL;
			queue_.pop_front();
		}
	}

	/* XXX Is not const because of the Mutex.  */
	bool empty(void)
	{
		ScopedLock _(&mtx_);
		return (queue_.empty());
	}

private:
	void cancel(CallbackAction *a)
	{
		/*
		 * XXX
		 * We need to synchronize access here.
		 *
		 * The problem is that a CallbackQueue may be deleted while a
		 * callback from it is scheduled but not executed.  Really, the
		 * structure here is a bit wrong, and perhaps drain should wait
		 * for all the actions to have been deleted?
		 */
		if (a->action_ != NULL) {
			a->action_->cancel();
			a->action_ = NULL;
			return;
		}

		ScopedLock _(&mtx_);
		std::deque<CallbackAction *>::iterator it;
		for (it = queue_.begin(); it != queue_.end(); ++it) {
			if (*it != a)
				continue;

			delete a->callback_;
			a->callback_ = NULL;

			queue_.erase(it);
			return;
		}
		NOTREACHED("/callback/queue");
	}
};

#endif /* !EVENT_CALLBACK_QUEUE_H */
