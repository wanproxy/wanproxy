#include <pthread.h>

#include <common/thread/mutex.h>
#include <common/thread/thread.h>

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

		Thread *self = Thread::self();
		ASSERT(self != NULL);
		ASSERT(owner_ == self);
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
	if (state_->owner_ != NULL) {
		HALT("/mutex") << "Attempt to lock already-owned mutex.";
		state_->unlock();
		return;
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
