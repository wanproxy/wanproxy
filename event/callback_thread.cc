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

#include <sched.h>

#include <event/callback_thread.h>

#include <event/event_callback.h>
#include <event/event_system.h>

CallbackThread::CallbackThread(const std::string& name)
: Thread(name),
  log_("/callback/thread/" + name),
  mtx_(name),
  sleepq_(name, &mtx_),
  idle_(false),
  queue_(),
  inflight_(NULL)
{ }

Action *
CallbackThread::schedule(CallbackBase *cb)
{
	mtx_.lock();
	bool need_wakeup = queue_.empty();
	queue_.push_back(cb);
	if (need_wakeup && idle_)
		sleepq_.signal();
	mtx_.unlock();

	return (cancellation(this, &CallbackThread::cancel, cb));
}

void
CallbackThread::cancel(CallbackBase *cb)
{
	Lock *interlock = cb->lock();
	ASSERT_LOCK_OWNED(log_, interlock);

	mtx_.lock();
	if (inflight_ == cb) {
		inflight_ = NULL;
		mtx_.unlock();
		delete cb;
		return;
	}

	std::deque<CallbackBase *>::iterator it;
	for (it = queue_.begin(); it != queue_.end(); ++it) {
		if (*it != cb)
			continue;
		queue_.erase(it);
		mtx_.unlock();
		delete cb;
		return;
	}

	NOTREACHED(log_);
}

void
CallbackThread::main(void)
{
	mtx_.lock();
	for (;;) {
		if (queue_.empty()) {
			idle_ = true;
			for (;;) {
				if (stop_) {
					mtx_.unlock();
					return;
				}
				sleepq_.wait();
				if (queue_.empty())
					continue;
				idle_ = false;
				break;
			}
		}

		while (!queue_.empty()) {
			CallbackBase *cb = queue_.front();
			queue_.pop_front();

			Lock *interlock = cb->lock();
			if (!interlock->try_lock()) {
				queue_.push_back(cb);

				/*
				 * Zip ahead to the next callback
				 * which has a different lock to
				 * prevent spinning on this one
				 * lock and livelocking.  When we
				 * get to one, or back to our
				 * initial callback, we can stop
				 * looking.  If we get all the
				 * way through the queue without
				 * finding a different lock, we
				 * might livelock and should do
				 * a yield after dropping our
				 * mutex.
				 *
				 * XXX This is slow and the
				 * algorithm is awful.  What would
				 * be better?
				 *
				 * At least this algorithm tends to
				 * avoid yielding gratuitously for
				 * very busy systems with several
				 * different targets for callbacks.
				 * It is pessimal only for really
				 * quite unlikely cases which are
				 * themselves already awful.
				 */
				CallbackBase *ncb;
				bool might_livelock = true;
				while ((ncb = queue_.front()) != cb) {
					if (ncb->lock() == interlock) {
						queue_.pop_front();
						queue_.push_back(ncb);
						continue;
					}
					might_livelock = false;
					break;
				}
				if (!might_livelock)
					continue;

				mtx_.unlock();
				sched_yield();
				mtx_.lock();
				continue;
			}
			inflight_ = cb;
			mtx_.unlock();

			cb->execute();
			interlock->unlock();

			/*
			 * Note:
			 * We do not acquire our mutex under
			 * the callback interlock, or we might
			 * never yield our mutex if we're
			 * livelocked with a single busy source
			 * of callbacks.
			 */

			mtx_.lock();
			if (inflight_ != NULL)
				HALT(log_) << "Callback not cancelled in execution.";
		}
	}
}
