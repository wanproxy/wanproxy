/*
 * Copyright (c) 2010-2011 Juli Mallett. All rights reserved.
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

#ifndef	COMMON_THREAD_THREAD_H
#define	COMMON_THREAD_THREAD_H

#include <common/thread/mutex.h>
#include <common/thread/sleep_queue.h>

struct ThreadState;

class Thread {
	friend struct ThreadState;

	std::string name_;
	ThreadState *state_;
protected:
	Mutex mtx_;
	SleepQueue sleepq_;
	bool signal_;
	bool stop_;

	Thread(const std::string&);

public:
	virtual ~Thread();

	void join(void);
	void start(void);

public:
	void main(void)
	{
		int ms;

		/*
		 * XXX
		 * Should carry around a deadline, not a maximum delay.
		 */
		ms = -1;

		mtx_.lock();
		while (!stop_) {
			if (!signal_) {
				ms = wait(ms);
				if (ms != 0)
					continue;
			}

			signal_ = false;
			mtx_.unlock();

			ms = work();

			mtx_.lock();
		}
		mtx_.unlock();
	}

	virtual int work(void) = 0;

	virtual int wait(int ms = -1)
	{
		ASSERT_LOCK_OWNED("/thread", &mtx_);
		ASSERT("/thread", !signal_);
		ASSERT("/thread", !stop_);
		return (sleepq_.wait(ms));
	}

	virtual void signal(void)
	{
		ScopedLock _(&mtx_);
		if (signal_)
			return;
		signal_ = true;
		sleepq_.signal();
	}

	virtual void stop(void)
	{
		ScopedLock _(&mtx_);
		if (stop_)
			return;
		stop_ = true;
		sleepq_.signal();
	}

	static Thread *self(void);
};

class NullThread : public Thread {
public:
	NullThread(const std::string& name)
	: Thread(name)
	{ }

	~NullThread()
	{ }

private:
	int work(void)
	{
		NOTREACHED("/thread/null");
	}
};

#endif /* !COMMON_THREAD_THREAD_H */
