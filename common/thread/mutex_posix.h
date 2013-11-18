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

#ifndef	COMMON_THREAD_MUTEX_POSIX_H
#define	COMMON_THREAD_MUTEX_POSIX_H

struct MutexState {
	pthread_mutex_t mutex_;
	pthread_mutexattr_t mutex_attr_;
	pthread_cond_t cond_;
	Thread::ID owner_;
	unsigned waiters_;

	MutexState(void)
	: mutex_(),
	  mutex_attr_(),
	  cond_(),
	  owner_(NULL),
	  waiters_(0)
	{
		int rv;

		rv = pthread_mutexattr_init(&mutex_attr_);
		ASSERT("/mutex/posix/state", rv != -1);

		rv = pthread_mutexattr_settype(&mutex_attr_, PTHREAD_MUTEX_RECURSIVE);
		ASSERT("/mutex/posix/state", rv != -1);

		rv = pthread_mutex_init(&mutex_, &mutex_attr_);
		ASSERT("/mutex/posix/state", rv != -1);

		rv = pthread_cond_init(&cond_, NULL);
	}

	~MutexState()
	{
		int rv;

		rv = pthread_mutex_destroy(&mutex_);
		ASSERT("/mutex/posix/state", rv != -1);

		rv = pthread_mutexattr_destroy(&mutex_attr_);
		ASSERT("/mutex/posix/state", rv != -1);

		rv = pthread_cond_destroy(&cond_);
		ASSERT("/mutex/posix/state", rv != -1);

#if 0 /* XXX What about extern Mutexes?  */
		Thread::ID self = Thread::selfID();
		ASSERT("/mutex/posix/state", self != NULL);
		ASSERT("/mutex/posix/state", owner_ == self);
#endif
	}

	/*
	 * Acquire the underlying lock.
	 */
	void lock(void)
	{
		int rv;

		rv = pthread_mutex_lock(&mutex_);
		ASSERT("/mutex/posix/state", rv != -1);
	}

	/*
	 * Acquire ownership of this mutex.
	 */
	void lock_acquire(void)
	{
		Thread::ID self = Thread::selfID();
		ASSERT("/mutex/posix/state", self != NULL);

		if (owner_ != NULL) {
			waiters_++;
			while (owner_ != NULL)
				pthread_cond_wait(&cond_, &mutex_);
			waiters_--;
		}
		owner_ = self;
	}

	/*
	 * Acquire ownership of this mutex without blocking.
	 */
	bool lock_acquire_try(void)
	{
		if (owner_ != NULL)
			return (false);

		Thread::ID self = Thread::selfID();
		ASSERT("/mutex/posix/state", self != NULL);
		owner_ = self;
		return (true);
	}

	/*
	 * Release the underlying lock.
	 */
	void unlock(void)
	{
		int rv;

		rv = pthread_mutex_unlock(&mutex_);
		ASSERT("/mutex/posix/state", rv != -1);
	}

	/*
	 * Release ownership of this Mutex.
	 */
	void lock_release(void)
	{
		Thread::ID self = Thread::selfID();
		ASSERT("/mutex/posix/state", self != NULL);

		if (owner_ == NULL) {
			HALT("/mutex/posix/state") << "Attempt to unlock already-unlocked mutex.";
			return;
		}
		ASSERT("/mutex/posix/state", owner_ == self);
		owner_ = NULL;

		if (waiters_ != 0)
			pthread_cond_signal(&cond_);
	}
};

#endif /* !COMMON_THREAD_MUTEX_POSIX_H */
