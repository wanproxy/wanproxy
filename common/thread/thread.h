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
	bool pending_;
	bool stop_;

	Thread(const std::string&);

public:
	virtual ~Thread();

	void join(void);
	void start(void);

public:
	void main(void)
	{
		mtx_.lock();
		while (!stop_) {
			if (pending_) {
				pending_ = false;
				mtx_.unlock();

				work();

				mtx_.lock();
				continue;
			}

			wait();
		}
		mtx_.unlock();

		final();
	}

	void submit(void)
	{
		signal(false);
	}

	void stop(void)
	{
		signal(true);
	}

protected:
	virtual void work(void) = 0;

	virtual void wait(void)
	{
		ASSERT_LOCK_OWNED("/thread", &mtx_);
		ASSERT("/thread", !pending_);
		ASSERT("/thread", !stop_);
		sleepq_.wait();
	}

	virtual void final(void)
	{
	}

	virtual void signal(bool stop)
	{
		ScopedLock _(&mtx_);
		if (!stop) {
			if (pending_)
				return;
			pending_ = true;
		} else {
			if (stop_)
				return;
			stop_ = true;
		}
		sleepq_.signal();
	}

public:
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
	void work(void)
	{
		NOTREACHED("/thread/null");
	}
};

#endif /* !COMMON_THREAD_THREAD_H */
