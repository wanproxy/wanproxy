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

#include <common/test.h>

#include <common/thread/mutex.h>
#include <common/thread/thread.h>

#define	NTHREAD		8
#define	ROUNDS		1024

class TestThread : public WorkerThread {
	Mutex test_mutex_;
	TestGroup& test_group_;
	Test *test_main_;
	Test *test_destroy_;
	Test test_ready_called_;
	Test test_ready_received_;
	bool test_ready_;
public:
	TestThread(TestGroup& test_group, Test *test_main, Test *test_destroy)
	: WorkerThread("TestThread"),
	  test_mutex_("TestMutex"),
	  test_group_(test_group),
	  test_main_(test_main),
	  test_destroy_(test_destroy),
	  test_ready_called_(test_group_, "TestThread::ready() called"),
	  test_ready_received_(test_group_, "TestThread::work() after TestThread::ready()"),
	  test_ready_(false)
	{ }

	~TestThread()
	{
		test_destroy_->pass();
	}

	void work(void)
	{
		test_main_->pass();

		ScopedLock _(&test_mutex_);
		if (test_ready_)
			test_ready_received_.pass();
		stop();
	}

	void ready(void)
	{
		ScopedLock _(&test_mutex_);
		test_ready_ = true;
		test_ready_called_.pass();
		submit();
	}
};

int
main(void)
{
	TestThread *threads[NTHREAD];
	Test *test_main[NTHREAD], *test_destroy[NTHREAD];

	TestGroup g("/test/thread/main1", "Thread::main #1");

	unsigned j;
	for (j = 0; j < ROUNDS; j++) {
		unsigned i;
		for (i = 0; i < NTHREAD; i++) {
			test_main[i] = new Test(g, "Main function called.");
			test_destroy[i] = new Test(g, "Destructor called.");
			threads[i] = new TestThread(g, test_main[i], test_destroy[i]);
		}

		for (i = 0; i < NTHREAD; i++)
			threads[i]->start();

		for (i = 0; i < NTHREAD; i++)
			threads[i]->ready();

		for (i = 0; i < NTHREAD; i++)
			threads[i]->join();

		for (i = 0; i < NTHREAD; i++)
			delete test_main[i];

		for (i = 0; i < NTHREAD; i++)
			delete threads[i];

		for (i = 0; i < NTHREAD; i++)
			delete test_destroy[i];
	}

	return (0);
}
