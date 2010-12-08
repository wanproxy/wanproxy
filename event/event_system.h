#ifndef	EVENT_SYSTEM_H
#define	EVENT_SYSTEM_H

#include <event/callback_queue.h>
#include <event/event.h>
#include <event/event_callback.h>
#include <event/event_poll.h>
#include <event/timeout_queue.h>

enum EventInterest {
	EventInterestReload,
	EventInterestStop
};

class EventSystem {
	LogHandle log_;
	CallbackQueue queue_;
	bool reload_;
	bool stop_;
	std::map<EventInterest, CallbackQueue> interest_queue_;
	TimeoutQueue timeout_queue_;
	EventPoll poll_;
protected:
	EventSystem(void);
	~EventSystem();

public:
	Action *poll(const EventPoll::Type&, int, EventCallback *);
	Action *register_interest(const EventInterest&, Callback *);
	Action *schedule(Callback *);
	Action *timeout(unsigned, Callback *);
	void start(void);
	void stop(void);

	void reload(void);

	static EventSystem *instance(void)
	{
		static EventSystem *instance_;

		if (instance_ == NULL)
			instance_ = new EventSystem();
		return (instance_);
	}
};

#endif /* !EVENT_SYSTEM_H */
