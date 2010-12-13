#include <pthread.h>

#include <common/thread/mutex.h>
#include <common/thread/thread.h>

#include "mutex_posix.h"

Mutex::Mutex(LockClass *lock_class, const std::string& name)
: Lock(lock_class, name),
  state_(new MutexState())
{ }

Mutex::~Mutex()
{
	if (state_ != NULL) {
		delete state_;
		state_ = NULL;
	}
}

void
Mutex::assert_owned(bool owned, const std::string& file, unsigned line, const std::string& function)
{
	Thread *self = Thread::self();
	ASSERT(self != NULL);

	state_->lock();
	if (state_->owner_ == NULL) {
		if (!owned) {
			state_->unlock();
			return;
		}
		HALT("/mutex") << "Lock at " << file << ":" << line << " is not owned; in function " << function;
		state_->unlock();
		return;
	}
	if (state_->owner_ == self) {
		if (owned) {
			state_->unlock();
			return;
		}
		HALT("/mutex") << "Lock at " << file << ":" << line << " is owned; in function " << function;
		state_->unlock();
		return;
	}
	if (!owned) {
		state_->unlock();
		return;
	}
	HALT("/mutex") << "Lock at " << file << ":" << line << " is owned by another thread; in function " << function;
	state_->unlock();
}

void
Mutex::lock(void)
{
	Thread *self = Thread::self();
	ASSERT(self != NULL);

	state_->lock();
	while (state_->owner_ != NULL) {
		state_->unlock();
		state_->lock();
	}
	state_->owner_ = self;
	state_->unlock();
}

void
Mutex::unlock(void)
{
	Thread *self = Thread::self();
	ASSERT(self != NULL);

	state_->lock();
	if (state_->owner_ == NULL) {
		HALT("/mutex") << "Attempt to unlock already-unlocked mutex.";
		state_->unlock();
		return;
	}
	ASSERT(state_->owner_ == self);
	state_->owner_ = NULL;
	state_->unlock();
}
