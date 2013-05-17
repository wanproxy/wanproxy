/*
 * Copyright (c) 2013 Juli Mallett. All rights reserved.
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
#include <event/timeout_thread.h>
#include <event/event_system.h>

TimeoutThread::TimeoutThread(void)
: WorkerThread("TimeoutThread"),
  log_("/event/timeout/thread"),
  timeout_queue_()
{
	INFO(log_) << "Starting timer thread.";
}

/*
 * XXX
 * Locking?
 */
void
TimeoutThread::work(void)
{
	if (!timeout_queue_.empty()) {
		/*
		 * XXX
		 * We need to manage the queue here and schedule the
		 * callbacks in the main EventThread, not just
		 * execute them in this thread directly.
		 */
		while (timeout_queue_.ready())
			timeout_queue_.perform();
	}
}

void
TimeoutThread::wait(void)
{
	if (timeout_queue_.empty()) {
		WorkerThread::wait();
		return;
	}
	NanoTime deadline = timeout_queue_.deadline();
	sleepq_.wait(&deadline);

	if (!pending_ && !timeout_queue_.empty() && timeout_queue_.ready())
		pending_ = true;
}
