#ifndef	LOCK_H
#define	LOCK_H

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
	LockClass *lock_class_;
	std::string name_;
protected:
	Lock(LockClass *lock_class, const std::string& name)
	: lock_class_(lock_class),
	  name_(name)
	{ }
public:
	virtual ~Lock()
	{ }

	virtual void assert_owned(bool, const std::string&, unsigned, const std::string&) = 0;
	virtual void lock(void) = 0;
	virtual void unlock(void) = 0;
};

#define	ASSERT_LOCK_OWNED(lock)						\
	((lock)->assert_owned(true, __FILE__, __LINE__, __PRETTY_FUNCTION__))
#define	ASSERT_LOCK_NOT_OWNED(lock)					\
	((lock)->assert_owned(false, __FILE__, __LINE__, __PRETTY_FUNCTION__))

#endif /* !LOCK_H */
