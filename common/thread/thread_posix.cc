#include <pthread.h>
#if defined(__FreeBSD__)
#include <pthread_np.h>
#endif

#include <set>

#include <common/thread/mutex.h>
#include <common/thread/sleep_queue.h>
#include <common/thread/thread.h>

#include "thread_posix.h"

static bool thread_posix_initialized;
static pthread_key_t thread_posix_key;

static LockClass thread_start_lock_class("Thread::start");
static Mutex thread_start_mutex(&thread_start_lock_class, "Thread::start");
static SleepQueue thread_start_sleepq("Thread::start", &thread_start_mutex);

static std::set<ThreadState *> running_threads;

static void thread_posix_init(void);
static void *thread_posix_start(void *);

static void thread_posix_signal_ignore(int);
static void thread_posix_signal_stop(int);

static NullThread initial_thread("initial thread");

bool Thread::stop_ = false;

Thread::Thread(const std::string& name)
: name_(name),
  state_(new ThreadState())
{ }

Thread::~Thread()
{
	if (state_ != NULL) {
		delete state_;
		state_ = NULL;
	}
}

void
Thread::join(void)
{
	void *val;
	int rv = pthread_join(state_->td_, &val);
	if (rv == -1) {
		ERROR("/thread/posix") << "Thread join failed.";
		return;
	}

	thread_start_mutex.lock();
	running_threads.erase(state_);
	thread_start_mutex.unlock();
}

void
Thread::start(void)
{
	if (!thread_posix_initialized) {
		thread_posix_init();
		if (!thread_posix_initialized) {
			ERROR("/thread/posix") << "Unable to initialize POSIX threads.";
			return;
		}
	}

	thread_start_mutex.lock();

	pthread_t td;
	int rv = pthread_create(&td, NULL, thread_posix_start, this);
	if (rv == -1) {
		ERROR("/thread/posix") << "Unable to start thread.";
		return;
	}

	thread_start_sleepq.wait();

	ASSERT(td == state_->td_);

	running_threads.insert(state_);

	thread_start_mutex.unlock();
}

Thread *
Thread::self(void)
{
	if (!thread_posix_initialized)
		thread_posix_init();
	ASSERT(thread_posix_initialized);

	void *ptr = pthread_getspecific(thread_posix_key);
	if (ptr == NULL)
		return (NULL);
	return ((Thread *)ptr);
}

static void
thread_posix_init(void)
{
	ASSERT(!thread_posix_initialized);

	signal(SIGINT, thread_posix_signal_stop);
	signal(SIGUSR1, thread_posix_signal_ignore);

	int rv = pthread_key_create(&thread_posix_key, NULL);
	if (rv == -1) {
		ERROR("/thread/posix/init") << "Could not initialize thread-local Thread pointer key.";
		return;
	}

	ThreadState::start(thread_posix_key, &initial_thread);

	thread_posix_initialized = true;
}

static void
thread_posix_signal_ignore(int)
{
	/* SIGUSR1 comes here so we can interrupt blocking syscalls.  */
}

static void
thread_posix_signal_stop(int sig)
{
	signal(sig, SIG_DFL);
	Thread::stop_ = true;

	INFO("/thread/posix/signal") << "Received SIGINT; setting stop flag.";

	pthread_t self = pthread_self();

	/*
	 * Forward signal to all threads.
	 *
	 * XXX Could deadlock.  Should try_lock.
	 */
	thread_start_mutex.lock();
	std::set<ThreadState *>::const_iterator it;
	for (it = running_threads.begin(); it != running_threads.end(); ++it) {
		const ThreadState *state = *it;

		/*
		 * Just send SIGUSR1 to interrupt any blocking syscalls.
		 */
		if (state->td_ == self)
			continue;
		pthread_kill(state->td_, SIGUSR1);
	}
	thread_start_mutex.unlock();
}

static void *
thread_posix_start(void *arg)
{
	Thread *td = (Thread *)arg;

	ThreadState::start(thread_posix_key, td);

	thread_start_mutex.lock();
	thread_start_sleepq.signal();
	thread_start_mutex.unlock();

	td->main();

	return (NULL);
}
