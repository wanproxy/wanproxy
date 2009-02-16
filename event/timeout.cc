#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/timeout.h>

Action *
TimeoutQueue::append(unsigned secs, Callback *cb)
{
	CallbackQueue& queue = timeout_queue_[time(NULL) + secs];
	Action *a = queue.append(cb);
	return (a);
}

time_t
TimeoutQueue::interval(void) const
{
	time_t now = time(NULL);
	timeout_map_t::const_iterator it = timeout_queue_.begin();
	if (it == timeout_queue_.end())
		return (0);
	if (it->first < now)
		return (0);
	return (it->first - now);
}

void
TimeoutQueue::perform(void)
{
	timeout_map_t::iterator it = timeout_queue_.begin();
	if (it == timeout_queue_.end())
		return;
	time_t now = time(NULL);
	if (it->first > now)
		return;
	CallbackQueue& queue = it->second;
	if (queue.empty())
		timeout_queue_.erase(it);
	else
		queue.perform();
}

bool
TimeoutQueue::ready(void) const
{
	timeout_map_t::const_iterator it = timeout_queue_.begin();
	if (it == timeout_queue_.end())
		return (false);
	time_t now = time(NULL);
	if (it->first <= now)
		return (true);
	return (false);
}
