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

	void lock(void)
	{
		int rv;

		rv = pthread_mutex_lock(&mutex_);
		ASSERT(rv != -1);
	}

	void unlock(void)
	{
		int rv;

		rv = pthread_mutex_unlock(&mutex_);
		ASSERT(rv != -1);
	}
};

#endif /* !MUTEX_POSIX_H */
