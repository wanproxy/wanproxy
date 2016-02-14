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
  queue_()
{ }

Action *
CallbackThread::schedule(CallbackBase *cb)
{
	ScopedLock _(&mtx_);
	bool need_wakeup = queue_.empty();
	queue_.push_back(cb);
	if (need_wakeup && idle_)
		sleepq_.signal();
	return (cb->scheduled(this));
}

void
CallbackThread::cancel(CallbackBase *cb)
{
	ScopedLock _(&mtx_);

	ASSERT_LOCK_OWNED(log_, cb->lock());

	std::deque<CallbackBase *>::iterator it;
	for (it = queue_.begin(); it != queue_.end(); ++it) {
		if (*it != cb)
			continue;
		queue_.erase(it);
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
				/*
				 * We cannot lock the callback.  We
				 * do not want to deadlock on its
				 * lock here, so push it to the back
				 * of the queue and deal with the
				 * next callback.
				 *
				 * Note that ideally we would zip
				 * through the whole list and look
				 * for any locks we *could* acquire,
				 * but for now we have an unlock and
				 * relock cycle with a yield in
				 * between to prevent livelock here.
				 *
				 * An earlier, naive algorithm tried
				 * to avoid yielding if there were
				 * other locks in use by callbacks
				 * in the list, but that's only safe
				 * if the number of threads is
				 * limited.  If all of the locks
				 * are held by active threads which
				 * are trying to acquire our lock,
				 * we would livelock.
				 *
				 * So for now go with the most
				 * conservative approach, which is
				 * to always yield.
				 */
				queue_.push_back(cb);

				mtx_.unlock();
				sched_yield();
				mtx_.lock();
				continue;
			}
			mtx_.unlock();

			cb->deschedule();
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
		}
	}
}
