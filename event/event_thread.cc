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
  reload_(),
  interest_queue_()
{
	INFO(log_) << "Starting event thread.";

	::signal(SIGHUP, signal_reload);
}

/*
 * XXX
 * Locking?
 */
void
EventThread::work(void)
{
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

		if (queue_.empty() || stop_ || reload_)
			break;

		queue_.drain();
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
	reload_ = true;
	submit();
}

namespace {
	static void
	signal_reload(int)
	{
		EventSystem::instance()->reload();
		/* XXX Forward signal to all threads.  */
	}
}
