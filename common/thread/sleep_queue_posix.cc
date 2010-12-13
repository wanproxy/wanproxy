#include <pthread.h>

#include <common/thread/mutex.h>
#include <common/thread/sleep_queue.h>
#include <common/thread/thread.h>

#include "mutex_posix.h"

struct SleepQueueState {
	pthread_cond_t cond_;
	MutexState *mutex_state_;

	SleepQueueState(MutexState *mutex_state)
	: cond_(),
	  mutex_state_(mutex_state)
	{
		int rv;

		rv = pthread_cond_init(&cond_, NULL);
		ASSERT(rv != -1);
	}

	~SleepQueueState()
	{
		int rv;

		rv = pthread_cond_destroy(&cond_);
		ASSERT(rv != -1);
	}

	void signal(void)
	{
		int rv;

		mutex_state_->lock();
		ASSERT(mutex_state_->owner_ == Thread::self());
		rv = pthread_cond_signal(&cond_);
		ASSERT(mutex_state_->owner_ == Thread::self());
		mutex_state_->unlock();
		ASSERT(rv != -1);
	}

	void wait(void)
	{
		int rv;

		mutex_state_->lock();
		mutex_state_->lock_release();
		rv = pthread_cond_wait(&cond_, &mutex_state_->mutex_);
		ASSERT(rv != -1);
		mutex_state_->lock_acquire();
		mutex_state_->unlock();
	}
};

SleepQueue::SleepQueue(const std::string& name, Mutex *mutex)
: name_(name),
  mutex_(mutex),
  state_(new SleepQueueState(mutex->state_))
{ }

SleepQueue::~SleepQueue()
{
#if 0 /* XXX What about extern SleepQueues?  */
	ASSERT_LOCK_OWNED(mutex_);
#endif

	if (state_ != NULL) {
		delete state_;
		state_ = NULL;
	}
}

void
SleepQueue::signal(void)
{
	ASSERT_LOCK_OWNED(mutex_);
	state_->signal();
	ASSERT_LOCK_OWNED(mutex_);
}

void
SleepQueue::wait(void)
{
	ASSERT_LOCK_OWNED(mutex_);
	state_->wait();
	ASSERT_LOCK_OWNED(mutex_);
}
