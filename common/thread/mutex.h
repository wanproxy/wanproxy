#ifndef	MUTEX_H
#define	MUTEX_H

#include <common/thread/lock.h>

struct MutexState;

class Mutex : public Lock {
	MutexState *state_;
public:
	Mutex(LockClass *, const std::string&);
	~Mutex();

	void assert_owned(bool, const std::string&, unsigned, const std::string&);
	void lock(void);
	void unlock(void);
};

#endif /* !MUTEX_H */
