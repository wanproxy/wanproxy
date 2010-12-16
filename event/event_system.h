#ifndef	EVENT_SYSTEM_H
#define	EVENT_SYSTEM_H

#include <event/event_thread.h>

/*
 * XXX
 * This is kind of an awful shim while we move
 * towards something thread-oriented.
 */

class EventSystem {
	EventThread td_;
private:
	EventSystem(void)
	: td_()
	{ }

	~EventSystem()
	{ }

public:
	Action *poll(const EventPoll::Type& type, int fd, EventCallback *cb)
	{
		return (td_.poll(type, fd, cb));
	}

	Action *register_interest(const EventInterest& interest, Callback *cb)
	{
		return (td_.register_interest(interest, cb));
	}

	Action *schedule(Callback *cb)
	{
		return (td_.schedule(cb));
	}

	Action *timeout(unsigned ms, Callback *cb)
	{
		return (td_.timeout(ms, cb));
	}

	void start(void)
	{
		td_.start();
	}

	void join(void)
	{
		td_.join();
	}

	static EventSystem *instance(void)
	{
		static EventSystem *instance;

		if (instance == NULL)
			instance = new EventSystem();
		return (instance);
	}
};

#endif /* !EVENT_SYSTEM_H */
