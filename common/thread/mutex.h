#ifndef	COMMON_THREAD_MUTEX_H
#define	COMMON_THREAD_MUTEX_H

#include <common/thread/lock.h>

struct MutexState;
class SleepQueue;

class Mutex : public Lock {
	friend class SleepQueue;

	MutexState *state_;
public:
	Mutex(LockClass *, const std::string&);
	~Mutex();

	void assert_owned(bool, const LogHandle&, const std::string&, unsigned, const std::string&);
	void lock(void);
	void unlock(void);
};

#endif /* !COMMON_THREAD_MUTEX_H */
