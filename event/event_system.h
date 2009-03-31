#ifndef	EVENT_SYSTEM_H
#define	EVENT_SYSTEM_H

#include <event/event.h>
#include <event/event_callback.h>
#include <event/event_poll.h>
#include <event/timeout.h>

class EventSystem {
	LogHandle log_;
	CallbackQueue queue_;
	bool stop_;
	CallbackQueue stop_queue_;
	TimeoutQueue timeout_queue_;
	EventPoll poll_;
protected:
	EventSystem(void);
	~EventSystem();

public:
	Action *poll(const EventPoll::Type&, int, EventCallback *);
	Action *schedule(Callback *);
	Action *schedule_stop(Callback *);
	Action *timeout(unsigned, Callback *);
	void start(void);
	void stop(void);

	static EventSystem *instance(void)
	{
		static EventSystem *instance_;

		if (instance_ == NULL)
			instance_ = new EventSystem();
		return (instance_);
	}
};

#endif /* !EVENT_SYSTEM_H */
