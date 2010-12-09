#include <pthread.h>
#if defined(__FreeBSD__)
#include <pthread_np.h>
#endif

#include <map>

#include <common/thread/thread.h>

struct ThreadPOSIX {
	Thread *td_;
	pthread_t ptd_;
};

static bool thread_posix_initialized;
static pthread_key_t thread_posix_key;
static std::map<Thread *, ThreadPOSIX *> thread_posix_map;

static void thread_posix_init(void);
static void *thread_posix_start(void *);

void
Thread::join(void)
{
	std::map<Thread *, ThreadPOSIX *>::iterator it;

	it = thread_posix_map.find(this);
	if (it == thread_posix_map.end()) {
		ERROR("/thread/posix") << "Attempt to join non-existant thread.";
		return;
	}

	void *val;
	ThreadPOSIX *td = it->second;
	int rv = pthread_join(td->ptd_, &val);
	if (rv == -1) {
		ERROR("/thread/posix") << "Thread join failed.";
		return;
	}

	thread_posix_map.erase(it);
	delete td;
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

	ThreadPOSIX *td = new ThreadPOSIX();
	td->td_ = this;

	thread_posix_map[td->td_] = td;

	int rv = pthread_create(&td->ptd_, NULL, thread_posix_start, td);
	if (rv == -1) {
		ERROR("/thread/posix") << "Unable to start thread.";
		delete td->td_;
		delete td;
		return;
	}

#if defined(__FreeBSD__)
	pthread_set_name_np(td->ptd_, td->td_->name_.c_str());
#endif
}

Thread *
Thread::self(void)
{
	ASSERT(thread_posix_initialized);

	void *ptr = pthread_getspecific(thread_posix_key);
	if (ptr == NULL)
		return (NULL);
	ThreadPOSIX *td = (ThreadPOSIX *)ptr;
	return (td->td_);
}

static void
thread_posix_init(void)
{
	ASSERT(!thread_posix_initialized);

	int rv = pthread_key_create(&thread_posix_key, NULL);
	if (rv == -1) {
		ERROR("/thread/posix/init") << "Could not initialize thread-local Thread pointer key.";
		return;
	}

	thread_posix_initialized = true;
}

static void *
thread_posix_start(void *arg)
{
	ThreadPOSIX *td = (ThreadPOSIX *)arg;

	int rv = pthread_setspecific(thread_posix_key, td);
	if (rv == -1) {
		ERROR("/thread/posix/start") << "Could not set thread-local Thread pointer.";
		return (NULL);
	}

	td->td_->main();

	return (NULL);
}
