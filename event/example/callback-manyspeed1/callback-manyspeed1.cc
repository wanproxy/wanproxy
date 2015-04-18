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

#include <common/thread/mutex.h>

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#define	CALLBACK_NUMBER	1000
#define	TIMER_MS	1000
#define	NTHREADS	8
#define	NMUTEXES	23

namespace {
	static EventThread *event_threads[NTHREADS];
}

class CallbackManySpeed {
	LogHandle log_;
	Mutex mtx_;
	uintmax_t callback_count_;
	Mutex *callback_mutex_[NMUTEXES];
	Action *callback_action_[CALLBACK_NUMBER];
	Action *timeout_action_;
public:
	CallbackManySpeed(void)
	: log_("/example/callback/manyspeed1"),
	  mtx_("CallbackManySpeed"),
	  callback_count_(0),
	  callback_action_(),
	  timeout_action_(NULL)
	{
		ScopedLock _(&mtx_);
		unsigned i;
		for (i = 0; i < NMUTEXES; i++) {
			callback_mutex_[i] = new Mutex("CallbackManySpeed::callback");
			callback_mutex_[i]->lock();
		}
		SimpleCallback *callbacks[CALLBACK_NUMBER];
		for (i = 0; i < CALLBACK_NUMBER; i++) {
			CallbackScheduler *scheduler = event_threads[i % NTHREADS];
			Mutex *mtx = callback_mutex_[i % NMUTEXES];
			SimpleCallback *cb = callback(scheduler, mtx, this, &CallbackManySpeed::callback_complete, i);
			callbacks[i] = cb;
		}
		for (i = 0; i < NMUTEXES; i++)
			callback_mutex_[i]->unlock();
		for (i = 0; i < CALLBACK_NUMBER; i++) {
			Mutex *mtx = callback_mutex_[i % NMUTEXES];
			mtx->lock();
			callback_action_[i] = callbacks[i]->schedule();
			mtx->unlock();
		}

		INFO(log_) << "Arming timer.";
		timeout_action_ = EventSystem::instance()->timeout(TIMER_MS, callback(&mtx_, this, &CallbackManySpeed::timer));
	}

	~CallbackManySpeed()
	{
		ASSERT(log_, timeout_action_ == NULL);
		unsigned i;
		for (i = 0; i < NMUTEXES; i++) {
			delete callback_mutex_[i];
			callback_mutex_[i] = NULL;
		}
	}

private:
	void callback_complete(unsigned i)
	{
		ASSERT_LOCK_OWNED(log_, callback_mutex_[i % NMUTEXES]);
		callback_action_[i]->cancel();
		callback_action_[i] = NULL;

		callback_count_++;

		CallbackScheduler *scheduler = event_threads[i % NTHREADS];
		Mutex *mtx = callback_mutex_[i % NMUTEXES];
		SimpleCallback *cb = callback(scheduler, mtx, this, &CallbackManySpeed::callback_complete, i);
		callback_action_[i] = cb->schedule();
	}

	void timer(void)
	{
		ASSERT_LOCK_OWNED(log_, &mtx_);
		timeout_action_->cancel();
		timeout_action_ = NULL;

		unsigned i;
		for (i = 0; i < CALLBACK_NUMBER; i++) {
			ScopedLock _(callback_mutex_[i % NMUTEXES]);
			ASSERT(log_, callback_action_[i] != NULL);
			callback_action_[i]->cancel();
			callback_action_[i] = NULL;
		}

		INFO(log_) << "Timer expired; " << callback_count_ << " callbacks.";

		EventSystem::instance()->stop();
	}
};

int
main(void)
{
	INFO("/example/callback/manyspeed1") << "Timer delay: " << TIMER_MS << "ms";

	unsigned i;
	for (i = 0; i < NTHREADS; i++) {
		EventThread *td = new EventThread();
		td->start();
		EventSystem::instance()->thread_wait(td);
		event_threads[i] = td;
	}

	CallbackManySpeed *cs = new CallbackManySpeed();

	event_main();

	delete cs;
}
