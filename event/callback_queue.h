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
#include <common/thread/sleep_queue.h>

#include <event/callback.h>
#include <event/cancellation.h>

class CallbackQueue : public CallbackScheduler {
	class CallbackAction : public Action {
	public:
		CallbackQueue *const queue_;
		CallbackBase *callback_;
		Action *action_;

		CallbackAction(CallbackQueue *queue, CallbackBase *callback)
		: queue_(queue),
		  callback_(callback),
		  action_(NULL)
		{ }

		~CallbackAction()
		{
			ASSERT_NULL("/callback/queue/action", callback_);
			ASSERT_NULL("/callback/queue/action", action_);
		}

		void cancel(void)
		{
			queue_->cancel(this);
			delete this;
		}
	};

	friend class CallbackAction;

	Mutex mtx_;
	SleepQueue sleepq_;
	std::deque<CallbackAction *> queue_;
	bool draining_;
public:
	CallbackQueue(void)
	: mtx_("CallbackQueue"),
	  sleepq_("CallbackQueue draining", &mtx_),
	  queue_(),
	  draining_(false)
	{ }

	~CallbackQueue()
	{
		ASSERT("/callback/queue", queue_.empty());
	}

	Action *schedule(CallbackBase *cb)
	{
		CallbackAction *a = new CallbackAction(this, cb);

		mtx_.lock();
		ASSERT("/callback/queue", !draining_);
		queue_.push_back(a);
		mtx_.unlock();

		return (a);
	}

	void drain(void)
	{
		std::deque<CallbackAction *>::iterator it;

		mtx_.lock();
		ASSERT("/callback/queue", !draining_);
		draining_ = true;
		for (it = queue_.begin(); it != queue_.end(); ++it) {
			CallbackAction *a = *it;
			ASSERT("/callback/queue", a->queue_ == this);
			a->action_ = a->callback_->schedule();
			a->callback_ = NULL;
		}

		/*
		 * The problem is that a CallbackQueue may be drained while a
		 * callback from it is scheduled but not executed.  So drain must
		 * wait for all the actions to have been cancelled before
		 * deleting the queue.
		 */
		while (!queue_.empty())
			sleepq_.wait();
		mtx_.unlock();
		delete this;
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
		ScopedLock _(&mtx_);
		std::deque<CallbackAction *>::iterator it;
		for (it = queue_.begin(); it != queue_.end(); ++it) {
			if (*it != a)
				continue;

			if (a->action_ != NULL) {
				a->action_->cancel();
				a->action_ = NULL;
				ASSERT_NULL("/callback/queue", a->callback_);
			} else {
				ASSERT_NON_NULL("/callback/queue", a->callback_);
				delete a->callback_;
				a->callback_ = NULL;
			}

			queue_.erase(it);

			if (draining_ && queue_.empty())
				sleepq_.signal();
			return;
		}
		NOTREACHED("/callback/queue");
	}
};

#endif /* !EVENT_CALLBACK_QUEUE_H */
