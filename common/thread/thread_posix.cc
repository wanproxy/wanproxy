/*
 * Copyright (c) 2010-2012 Juli Mallett. All rights reserved.
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

#include <pthread.h>
#if defined(__FreeBSD__)
#include <pthread_np.h>
#endif
#include <signal.h>

#include <set>

#include <common/thread/mutex.h>
#include <common/thread/sleep_queue.h>
#include <common/thread/thread.h>

#include "thread_posix.h"

namespace {
	static bool thread_posix_initialized;
	static pthread_key_t thread_posix_key;

	static Mutex thread_start_mutex("Thread::start");
	static SleepQueue thread_start_sleepq("Thread::start", &thread_start_mutex);

	static std::set<Thread *> running_threads;

	static void thread_posix_init(void);
	static void *thread_posix_start(void *);

	static void thread_posix_signal_ignore(int);

	static NullThread initial_thread("initial thread");
}

Thread::Thread(const std::string& name)
: name_(name),
  state_(new ThreadState()),
  mtx_("Thread"),
  sleepq_("Thread", &mtx_),
  signal_(false),
  stop_(false)
{ }

Thread::~Thread()
{
	if (state_ != NULL) {
		delete state_;
		state_ = NULL;
	}
}

void
Thread::join(void)
{
	void *val;
	int rv = pthread_join(state_->td_, &val);
	if (rv == -1) {
		ERROR("/thread/posix") << "Thread join failed.";
		return;
	}

	thread_start_mutex.lock();
	running_threads.erase(this);
	thread_start_mutex.unlock();
}

void
Thread::start(void)
{
	if (!thread_posix_initialized) {
		thread_posix_init();
		if (!thread_posix_initialized) {
			ERROR("/thread/posix") << "Unable to initialize POSIX threads.";
			return;
		}
	}

	thread_start_mutex.lock();

	pthread_t td;
	int rv = pthread_create(&td, NULL, thread_posix_start, this);
	if (rv == -1) {
		ERROR("/thread/posix") << "Unable to start thread.";
		return;
	}

	thread_start_sleepq.wait();

	ASSERT("/thread/posix", td == state_->td_);

	running_threads.insert(this);

	thread_start_mutex.unlock();
}

Thread *
Thread::self(void)
{
	if (!thread_posix_initialized)
		thread_posix_init();
	ASSERT("/thread/posix", thread_posix_initialized);

	void *ptr = pthread_getspecific(thread_posix_key);
	if (ptr == NULL)
		return (NULL);
	return ((Thread *)ptr);
}

void
ThreadState::signal_stop(int sig)
{
	signal(sig, SIG_DFL);

	INFO("/thread/posix/signal") << "Received SIGINT; setting stop flag.";

	pthread_t self = pthread_self();

	/*
	 * Forward signal to all threads.
	 *
	 * XXX Could deadlock.  Should try_lock.
	 */
	thread_start_mutex.lock();
	std::set<Thread *>::const_iterator it;
	for (it = running_threads.begin(); it != running_threads.end(); ++it) {
		Thread *td = *it;
		ThreadState *state = td->state_;

		if (state->td_ == self) {
			td->stop_ = true;
			continue;
		}

		td->stop();

		/*
		 * Also send SIGUSR1 to interrupt any blocking syscalls.
		 */
		pthread_kill(state->td_, SIGUSR1);
	}
	thread_start_mutex.unlock();
}

namespace {
	static void
	thread_posix_init(void)
	{
		ASSERT("/thread/posix", !thread_posix_initialized);

		signal(SIGINT, ThreadState::signal_stop);
		signal(SIGUSR1, thread_posix_signal_ignore);

		int rv = pthread_key_create(&thread_posix_key, NULL);
		if (rv == -1) {
			ERROR("/thread/posix/init") << "Could not initialize thread-local Thread pointer key.";
			return;
		}

		ThreadState::start(thread_posix_key, &initial_thread);

		thread_posix_initialized = true;
	}

	static void
	thread_posix_signal_ignore(int)
	{
		/* SIGUSR1 comes here so we can interrupt blocking syscalls.  */
	}

	static void *
	thread_posix_start(void *arg)
	{
		Thread *td = (Thread *)arg;

		ThreadState::start(thread_posix_key, td);

		thread_start_mutex.lock();
		thread_start_sleepq.signal();
		thread_start_mutex.unlock();

		td->main();

		return (NULL);
	}
}
