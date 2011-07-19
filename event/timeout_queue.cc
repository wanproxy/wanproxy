#include <common/time/time.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/callback_queue.h>
#include <event/timeout_queue.h>

Action *
TimeoutQueue::append(uintmax_t ms, SimpleCallback *cb)
{
	NanoTime now = NanoTime::current_time();

	now.seconds_ += ms / 1000;
	now.nanoseconds_ += (ms % 1000) * 1000000;

	CallbackQueue& queue = timeout_queue_[now];
	Action *a = queue.schedule(cb);
	return (a);
}

uintmax_t
TimeoutQueue::interval(void) const
{
	NanoTime now = NanoTime::current_time();
	timeout_map_t::const_iterator it = timeout_queue_.begin();
	if (it == timeout_queue_.end())
		return (0);
	if (it->first < now)
		return (0);

	NanoTime expiry = it->first;
	expiry -= now;
	return ((expiry.seconds_ * 1000) + (expiry.nanoseconds_ / 1000000));
}

void
TimeoutQueue::perform(void)
{
	timeout_map_t::iterator it = timeout_queue_.begin();
	if (it == timeout_queue_.end())
		return;
	NanoTime now = NanoTime::current_time();
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
	NanoTime now = NanoTime::current_time();
	if (it->first <= now)
		return (true);
	return (false);
}
