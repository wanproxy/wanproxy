#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/callback_queue.h>
#include <event/timeout_queue.h>

TimeoutQueue::NanoTime
TimeoutQueue::NanoTime::current_time(void)
{
	NanoTime nt;
	int rv;

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
	struct timespec ts;

	rv = ::clock_gettime(CLOCK_MONOTONIC, &ts);
	ASSERT(rv != -1);

	nt.seconds_ = ts.tv_sec;
	nt.nanoseconds_ = ts.tv_nsec;
#else
	struct timeval tv;

	rv = ::gettimeofday(&tv, NULL);
	ASSERT(rv != -1);
	nt.seconds_ = tv.tv_sec;
	nt.nanoseconds_ = tv.tv_usec * 1000;
#endif

	return (nt);
}

Action *
TimeoutQueue::append(uintmax_t ms, Callback *cb)
{
	TimeoutQueue::NanoTime now = TimeoutQueue::NanoTime::current_time();

	now.seconds_ += ms / 1000;
	now.nanoseconds_ += (ms % 1000) * 1000000;

	CallbackQueue& queue = timeout_queue_[now];
	Action *a = queue.append(cb);
	return (a);
}

uintmax_t
TimeoutQueue::interval(void) const
{
	TimeoutQueue::NanoTime now = TimeoutQueue::NanoTime::current_time();
	timeout_map_t::const_iterator it = timeout_queue_.begin();
	if (it == timeout_queue_.end())
		return (0);
	if (it->first < now)
		return (0);

	TimeoutQueue::NanoTime expiry = it->first;
	expiry -= now;
	return ((expiry.seconds_ * 1000) + (expiry.nanoseconds_ / 1000000));
}

void
TimeoutQueue::perform(void)
{
	timeout_map_t::iterator it = timeout_queue_.begin();
	if (it == timeout_queue_.end())
		return;
	TimeoutQueue::NanoTime now = TimeoutQueue::NanoTime::current_time();
	if (it->first > now)
		return;
	CallbackQueue& queue = it->second;
	while (!queue.empty())
		queue.perform();
	timeout_queue_.erase(it);
}

bool
TimeoutQueue::ready(void) const
{
	timeout_map_t::const_iterator it = timeout_queue_.begin();
	if (it == timeout_queue_.end())
		return (false);
	TimeoutQueue::NanoTime now = TimeoutQueue::NanoTime::current_time();
	if (it->first <= now)
		return (true);
	return (false);
}
