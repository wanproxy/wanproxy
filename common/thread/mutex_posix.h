#ifndef	MUTEX_POSIX_H
#define	MUTEX_POSIX_H

struct MutexState {
	pthread_mutex_t mutex_;
	pthread_mutexattr_t mutex_attr_;
	Thread *owner_;

	MutexState(void)
	: mutex_(),
	  mutex_attr_(),
	  owner_(NULL)
	{
		int rv;

		rv = pthread_mutexattr_init(&mutex_attr_);
		ASSERT(rv != -1);

		rv = pthread_mutexattr_settype(&mutex_attr_, PTHREAD_MUTEX_RECURSIVE);
		ASSERT(rv != -1);

		rv = pthread_mutex_init(&mutex_, &mutex_attr_);
		ASSERT(rv != -1);
	}

	~MutexState()
	{
		int rv;

		rv = pthread_mutex_destroy(&mutex_);
		ASSERT(rv != -1);

		rv = pthread_mutexattr_destroy(&mutex_attr_);
		ASSERT(rv != -1);

#if 0 /* XXX What about extern Mutexes?  */
		Thread *self = Thread::self();
		ASSERT(self != NULL);
		ASSERT(owner_ == self);
#endif
	}

	/*
	 * Acquire the underlying lock.
	 */
	void lock(void)
	{
		int rv;

		rv = pthread_mutex_lock(&mutex_);
		ASSERT(rv != -1);
	}

	/*
	 * Acquire ownership of this mutex.
	 */
	void lock_acquire(void)
	{
		Thread *self = Thread::self();
		ASSERT(self != NULL);

		while (owner_ != NULL) {
			unlock();
			/* XXX Wait on a condvar instead?  */
			lock();
		}
		owner_ = self;
	}

	/*
	 * Release the underlying lock.
	 */
	void unlock(void)
	{
		int rv;

		rv = pthread_mutex_unlock(&mutex_);
		ASSERT(rv != -1);
	}

	/*
	 * Release ownership of this Mutex.
	 */
	void lock_release(void)
	{
		Thread *self = Thread::self();
		ASSERT(self != NULL);

		if (owner_ == NULL) {
			HALT("/mutex") << "Attempt to unlock already-unlocked mutex.";
			return;
		}
		ASSERT(owner_ == self);
		owner_ = NULL;
	}
};

#endif /* !MUTEX_POSIX_H */
