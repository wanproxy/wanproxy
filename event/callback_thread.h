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

#ifndef	EVENT_CALLBACK_THREAD_H
#define	EVENT_CALLBACK_THREAD_H

#include <deque>

#include <common/thread/thread.h>

#include <event/callback.h>

/* XXX Convert from WorkerThread to Thread so we can make stop() DTRT.  */
class CallbackThread : public Thread, public CallbackScheduler {
protected:
	LogHandle log_;
private:
	Mutex mtx_;
	SleepQueue sleepq_;
	bool idle_;
	std::deque<CallbackBase *> queue_;
	CallbackBase *inflight_;
public:
	CallbackThread(const std::string&);

	~CallbackThread()
	{ }

	Action *schedule(CallbackBase *);

private:
	void cancel(CallbackBase *);

	void main(void);

public:
	virtual void stop(void)
	{
		ScopedLock _(&mtx_);
		if (stop_)
			return;
		stop_ = true;
		sleepq_.signal();
	}
};

#endif /* !EVENT_CALLBACK_THREAD_H */
