#ifndef	THREAD_POSIX_H
#define	THREAD_POSIX_H

struct ThreadState {
	pthread_t td_;

	static void start(pthread_key_t key, Thread *td)
	{
		pthread_t self = pthread_self();

		td->state_->td_ = self;

		int rv = pthread_setspecific(key, td);
		if (rv == -1) {
			ERROR("/thread/state/start") << "Could not set thread-local Thread pointer.";
			return;
		}

#if defined(__FreeBSD__)
		pthread_set_name_np(self, td->name_.c_str());
#elif defined(__APPLE__)
		pthread_setname_np(td->name_.c_str());
#endif
	}
};



#endif /* !THREAD_POSIX_H */
