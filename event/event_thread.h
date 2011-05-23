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
	std::map<EventInterest, CallbackQueue> interest_queue_;
	TimeoutQueue timeout_queue_;
	EventPoll poll_;
public:
	EventThread(void);
	~EventThread();

public:
	Action *poll(const EventPoll::Type& type, int fd, EventCallback *cb)
	{
		Action *a = poll_.poll(type, fd, cb);
		return (a);
	}

	Action *register_interest(const EventInterest& interest, SimpleCallback *cb)
	{
		Action *a = interest_queue_[interest].schedule(cb);
		return (a);
	}

	Action *schedule(CallbackBase *cb)
	{
		Action *a = queue_.schedule(cb);
		return (a);
	}

	Action *timeout(unsigned secs, SimpleCallback *cb)
	{
		Action *a = timeout_queue_.append(secs, cb);
		return (a);
	}

private:
	void main(void);
public:
	void reload(void);

	static EventThread *self(void)
	{
		Thread *td = Thread::self();
		return (dynamic_cast<EventThread *>(td));
	}
};

#endif /* !EVENT_THREAD_H */
