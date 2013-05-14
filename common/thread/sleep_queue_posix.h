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

#ifndef	COMMON_THREAD_SLEEP_QUEUE_POSIX_H
#define	COMMON_THREAD_SLEEP_QUEUE_POSIX_H

#include "mutex_posix.h"

struct SleepQueueState {
	pthread_cond_t cond_;
	MutexState *mutex_state_;

	SleepQueueState(MutexState *mutex_state)
	: cond_(),
	  mutex_state_(mutex_state)
	{
		int rv;

		rv = pthread_cond_init(&cond_, NULL);
		ASSERT("/sleep/queue/posix/state", rv != -1);
	}

	~SleepQueueState()
	{
		int rv;

		rv = pthread_cond_destroy(&cond_);
		ASSERT("/sleep/queue/posix/state", rv != -1);
	}

	void signal(void)
	{
		int rv;

		mutex_state_->lock();
		ASSERT("/sleep/queue/posix/state", mutex_state_->owner_ == Thread::self());
		rv = pthread_cond_signal(&cond_);
		ASSERT("/sleep/queue/posix/state", mutex_state_->owner_ == Thread::self());
		mutex_state_->unlock();
		ASSERT("/sleep/queue/posix/state", rv != -1);
	}

	void wait(void)
	{
		int rv;

		mutex_state_->lock();
		mutex_state_->lock_release();
		rv = pthread_cond_wait(&cond_, &mutex_state_->mutex_);
		ASSERT("/sleep/queue/posix/state", rv != -1);
		mutex_state_->lock_acquire();
		mutex_state_->unlock();
	}
};

#endif /* !COMMON_THREAD_SLEEP_QUEUE_POSIX_H */
