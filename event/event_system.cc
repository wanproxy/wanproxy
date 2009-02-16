#include <unistd.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

EventSystem::EventSystem(void)
: log_("/event/system"),
  queue_(),
  poll_()
{
	INFO(log_) << "Starting event system.";
}

EventSystem::~EventSystem()
{
}

Action *
EventSystem::poll(const EventPoll::Type& type, int fd, EventCallback *cb)
{
	Action *a = poll_.poll(type, fd, cb);
	return (a);
}

Action *
EventSystem::schedule(Callback *cb)
{
	Action *a = queue_.append(cb);
	return (a);
}

Action *
EventSystem::timeout(unsigned secs, Callback *cb)
{
	Action *a = timeout_queue_.append(secs, cb);
	return (a);
}

void
EventSystem::start(void)
{
	for (;;) {
		/*
		 * If there are time-triggered events whose time has come,
		 * run them.
		 */
		while (timeout_queue_.ready())
			timeout_queue_.perform();
		/*
		 * And then run a pending callback.
		 */
		queue_.perform();
		/*
		 * And if there are more pending callbacks, then do a quick
		 * poll and let them run.
		 */
		if (!queue_.empty()) {
			poll_.poll();
			continue;
		}
		/*
		 * But if there are no pending callbacks, and no timers or
		 * file descriptors being polled, stop.
		 */
		if (timeout_queue_.empty() && poll_.idle())
			break;
		/*
		 * If there are timers ticking down, then let the poll module
		 * block until a timer will be ready.
		 * XXX Maybe block 1 second less, just in case?  We'll never
		 * be a realtime system though, so any attempt at pretending
		 * to be one, beyond giving time events priority at the top of
		 * this loop, is probably a mistake.
		 */
		if (!timeout_queue_.empty()) {
			poll_.wait(timeout_queue_.interval());
			continue;
		}
		/*
		 * If, however, there's nothing sensitive to time, but someone
		 * has a file descriptor that can be blocked on, just block on it
		 * indefinitely.
		 */
		poll_.wait();
	}
}
