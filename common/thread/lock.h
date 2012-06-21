#ifndef	COMMON_THREAD_LOCK_H
#define	COMMON_THREAD_LOCK_H

class LockClass {
	std::string name_;
public:
	LockClass(const std::string& name)
	: name_(name)
	{ }

	~LockClass()
	{ }
};

class Lock {
	std::string name_;
protected:
	Lock(LockClass *, const std::string& name)
	: name_(name)
	{ }
public:
	virtual ~Lock()
	{ }

	virtual void assert_owned(bool, const LogHandle&, const std::string&, unsigned, const std::string&) = 0;
	virtual void lock(void) = 0;
	virtual void unlock(void) = 0;
};

#define	ASSERT_LOCK_OWNED(log, lock)					\
	((lock)->assert_owned(true, log, __FILE__, __LINE__, __PRETTY_FUNCTION__))
#define	ASSERT_LOCK_NOT_OWNED(log, lock)				\
	((lock)->assert_owned(false, log, __FILE__, __LINE__, __PRETTY_FUNCTION__))

class ScopedLock {
	Lock *lock_;
public:
	ScopedLock(Lock *lock)
	: lock_(lock)
	{
		ASSERT("/scoped/lock", lock_ != NULL);
		lock_->lock();
	}

	~ScopedLock()
	{
		if (lock_ != NULL) {
			lock_->unlock();
			lock_ = NULL;
		}
	}

	void acquire(Lock *lock)
	{
		ASSERT("/scoped/lock", lock_ == NULL);
		lock_ = lock;
		lock_->lock();
	}

	void drop(void)
	{
		ASSERT("/scoped/lock", lock_ != NULL);
		lock_->unlock();
		lock_ = NULL;
	}
};

#endif /* !COMMON_THREAD_LOCK_H */
