/*
 * Copyright (c) 2010-2014 Juli Mallett. All rights reserved.
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

#ifndef	COMMON_THREAD_SLEEP_QUEUE_POSIX_H
#define	COMMON_THREAD_SLEEP_QUEUE_POSIX_H

#include <unistd.h> /* For _POSIX_TIMERS */

#include <common/time/time.h>

#include "mutex_posix.h"

struct SleepQueueState {
	pthread_cond_t cond_;
	MutexState *mutex_state_;
	bool waiting_;

	SleepQueueState(MutexState *mutex_state)
	: cond_(),
	  mutex_state_(mutex_state),
	  waiting_(false)
	{
		pthread_condattr_t attr;
		int error;

		error = pthread_condattr_init(&attr);
		ASSERT_ZERO("/sleep/queue/posix/state", error);

		/* Match behaviour/clock of NanoTime.  */
#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
		error = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
		ASSERT_ZERO("/sleep/queue/posix/state", error);
#endif

		error = pthread_cond_init(&cond_, &attr);
		ASSERT_ZERO("/sleep/queue/posix/state", error);

		error = pthread_condattr_destroy(&attr);
		ASSERT_ZERO("/sleep/queue/posix/state", error);
	}

	~SleepQueueState()
	{
		int error;

		error = pthread_cond_destroy(&cond_);
		ASSERT_ZERO("/sleep/queue/posix/state", error);
	}

	void signal(void)
	{
		int error;

		if (!waiting_)
			return;

		mutex_state_->lock();
#ifndef NDEBUG
		ASSERT("/sleep/queue/posix/state", mutex_state_->owner_ == Thread::selfID());
#endif
		error = pthread_cond_signal(&cond_);
#ifndef NDEBUG
		ASSERT("/sleep/queue/posix/state", mutex_state_->owner_ == Thread::selfID());
#endif
		mutex_state_->unlock();
		ASSERT_ZERO("/sleep/queue/posix/state", error);
	}

	void wait(const NanoTime *deadline)
	{
		struct timespec ts;
		int error;

		if (deadline != NULL) {
			ts.tv_sec = deadline->seconds_;
			ts.tv_nsec = deadline->nanoseconds_;
		}

		waiting_ = true;

#ifndef NDEBUG
		mutex_state_->lock();
		mutex_state_->lock_release();
#endif
		if (deadline == NULL) {
			error = pthread_cond_wait(&cond_, &mutex_state_->mutex_);
			ASSERT_ZERO("/sleep/queue/posix/state", error);
		} else {
			error = pthread_cond_timedwait(&cond_, &mutex_state_->mutex_, &ts);
			ASSERT("/sleep/queue/posix/state", error == 0 || error == ETIMEDOUT);
		}
#ifndef NDEBUG
		mutex_state_->lock_acquire();
		mutex_state_->unlock();
#endif

		waiting_ = false;
	}
};

#endif /* !COMMON_THREAD_SLEEP_QUEUE_POSIX_H */
