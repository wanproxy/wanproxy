/*
 * Copyright (c) 2008-2012 Juli Mallett. All rights reserved.
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
: Thread("EventThread"),
  log_("/event/thread"),
  queue_(),
  reload_(),
  interest_queue_(),
  timeout_queue_(),
  poll_()
{
	INFO(log_) << "Starting event thread.";

	::signal(SIGHUP, signal_reload);
	signal_ = true;
}

/*
 * XXX
 * Locking?
 */
int
EventThread::work(void)
{
	for (;;) {
		/*
		 * If we have been told to reload, fire all shutdown events.
		 */
		if (reload_ && !interest_queue_[EventInterestReload].empty()) {
			INFO(log_) << "Running reload handlers.";
			interest_queue_[EventInterestReload].drain();
			INFO(log_) << "Reload handlers have been run.";

			reload_ = false;
			::signal(SIGHUP, signal_reload);
		}

		/*
		 * If there are time-triggered events whose time has come,
		 * run them.
		 */
		if (!timeout_queue_.empty()) {
			while (timeout_queue_.ready())
				timeout_queue_.perform();
		}

		while (!timeout_queue_.ready()) {
			if (queue_.empty() || stop_ || reload_)
				break;
			queue_.perform();
		}

		if (queue_.empty() || stop_ || reload_)
			break;
	}

	if (timeout_queue_.empty())
		return (-1);
	return (timeout_queue_.interval());
#if 0
	for (;;) {
		/*
		 * If we have been told to stop, fire all shutdown events.
		 */
		if (stop_ && !interest_queue_[EventInterestStop].empty()) {
			INFO(log_) << "Running stop handlers.";
			if (interest_queue_[EventInterestStop].drain())
				ERROR(log_) << "Stop handlers added other stop handlers.";
			INFO(log_) << "Stop handlers have been run.";
		}



		/*
		 * If there are any pending callbacks, go back to the start
		 * of the loop.
		 */
		if (!queue_.empty() || timeout_queue_.ready())
			continue;

		/*
		 * But if there are no pending callbacks, and no timers or
		 * file descriptors being polled, stop.
		 */
		if (timeout_queue_.empty() && poll_.idle())
			break;
		/*
		 * If there are timers ticking down, then let the poll module
		 * block until a timer will be ready.
		 * XXX Maybe block 1 second less, just in case?  We'll never
		 * be a realtime thread though, so any attempt at pretending
		 * to be one, beyond giving time events priority at the top of
		 * this loop, is probably a mistake.
		 */
		if (!timeout_queue_.empty()) {
			poll_.wait(timeout_queue_.interval());
			continue;
		}
		/*
		 * If, however, there's nothing sensitive to time, but someone
		 * has a file descriptor that can be blocked on, just block on it
		 * indefinitely.
		 */
	}
#endif
}

void
EventThread::reload(void)
{
	::signal(SIGHUP, SIG_IGN);
	INFO(log_) << "Running reload events.";
	reload_ = true;
	signal();
}

namespace {
	static void
	signal_reload(int)
	{
		EventSystem::instance()->reload();
		/* XXX Forward signal to all threads.  */
	}
}
