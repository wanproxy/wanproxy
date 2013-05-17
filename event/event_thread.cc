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

#include <signal.h>
#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_thread.h>
#include <event/event_system.h>

namespace {
	static void signal_reload(int);
}

EventThread::EventThread(void)
: WorkerThread("EventThread"),
  log_("/event/thread"),
  queue_(),
  inflight_(NULL)
#if 0
  reload_(),
  interest_queue_()
#endif
{
	::signal(SIGHUP, signal_reload);
}

/*
 * NB:
 * Unless the caller itself is running in the EventThread, it needs to
 * acquire a lock in its cancel path as well as around this schedule
 * call to avoid races when dispatching deferred callbacks.
 */
Action *
EventThread::schedule(CallbackBase *cb)
{
	mtx_.lock();
	bool need_wakeup = queue_.empty();
	queue_.push_back(cb);
	if (need_wakeup && !pending_) {
		pending_ = true;
		sleepq_.signal();
	}
	mtx_.unlock();

	return (cancellation(this, &EventThread::cancel, cb));
}

void
EventThread::cancel(CallbackBase *cb)
{
	mtx_.lock();
	if (inflight_ == cb) {
		inflight_ = NULL;
		mtx_.unlock();
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

/*
 * XXX
 * Locking?
 */
void
EventThread::work(void)
{
	mtx_.lock();
	for (;;) {
		/*
		 * If we have been told to reload, fire all shutdown events.
		 */
#if 0
		if (reload_ && !interest_queue_[EventInterestReload].empty()) {
			INFO(log_) << "Running reload handlers.";
			interest_queue_[EventInterestReload].drain();
			INFO(log_) << "Reload handlers have been run.";

			reload_ = false;
			::signal(SIGHUP, signal_reload);
		}
#endif

		if (queue_.empty() || stop_) {
			mtx_.unlock();
			return;
		}

		CallbackBase *cb = queue_.front();
		queue_.pop_front();
		inflight_ = cb;
		mtx_.unlock();

		cb->execute();
		delete cb;

		mtx_.lock();
		if (inflight_ != NULL)
			HALT(log_) << "Callback not cancelled in execution.";
	}
}

void
EventThread::final(void)
{
#if 0
	/*
	 * If we have been told to stop, fire all shutdown events.
	 */
	if (!interest_queue_[EventInterestStop].empty()) {
		INFO(log_) << "Running stop handlers.";
		if (interest_queue_[EventInterestStop].drain())
			ERROR(log_) << "Stop handlers added other stop handlers.";
		INFO(log_) << "Stop handlers have been run.";
	}
#endif
}

void
EventThread::reload(void)
{
	::signal(SIGHUP, SIG_IGN);
	INFO(log_) << "Running reload events.";
#if 0
	reload_ = true;
	submit();
#endif
}

namespace {
	static void
	signal_reload(int)
	{
		EventSystem::instance()->reload();
		/* XXX Forward signal to all threads.  */
	}
}
