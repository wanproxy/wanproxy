#ifndef	COMMON_THREAD_MUTEX_POSIX_H
#define	COMMON_THREAD_MUTEX_POSIX_H

struct MutexState {
	pthread_mutex_t mutex_;
	pthread_mutexattr_t mutex_attr_;
	pthread_cond_t cond_;
	Thread *owner_;

	MutexState(void)
	: mutex_(),
	  mutex_attr_(),
	  cond_(),
	  owner_(NULL)
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
		Thread *self = Thread::self();
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
		Thread *self = Thread::self();
		ASSERT("/mutex/posix/state", self != NULL);

		while (owner_ != NULL)
			pthread_cond_wait(&cond_, &mutex_);
		owner_ = self;
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
		Thread *self = Thread::self();
		ASSERT("/mutex/posix/state", self != NULL);

		if (owner_ == NULL) {
			HALT("/mutex/posix/state") << "Attempt to unlock already-unlocked mutex.";
			return;
		}
		ASSERT("/mutex/posix/state", owner_ == self);
		owner_ = NULL;

		pthread_cond_signal(&cond_);
	}
};

#endif /* !COMMON_THREAD_MUTEX_POSIX_H */
