#ifndef	EVENT_THREAD_H
#define	EVENT_THREAD_H

#include <common/thread/thread.h>
#include <event/callback_queue.h>
#include <event/event_poll.h>
#include <event/timeout_queue.h>

enum EventInterest {
	EventInterestReload,
	EventInterestStop
};

class EventThread : public Thread {
	LogHandle log_;
	CallbackQueue queue_;
	bool reload_;
	bool stop_;
	std::map<EventInterest, CallbackQueue> interest_queue_;
	TimeoutQueue timeout_queue_;
	EventPoll poll_;
public:
	EventThread(void);
	~EventThread();

public:
	Action *poll(const EventPoll::Type&, int, EventCallback *);
	Action *register_interest(const EventInterest&, Callback *);
	Action *schedule(Callback *);
	Action *timeout(unsigned, Callback *);
private:
	void main(void);
public:
	void stop(void);

	void reload(void);

	static EventThread *self(void)
	{
		Thread *td = Thread::self();
		return (dynamic_cast<EventThread *>(td));
	}
};

#endif /* !EVENT_THREAD_H */
