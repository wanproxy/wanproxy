#ifndef	EVENT_EVENT_POLL_H
#define	EVENT_EVENT_POLL_H

#include <map>

struct EventPollState;

class EventPoll {
	friend class EventThread;
	friend class PollAction;

public:
	enum Type {
		Readable,
		Writable,
	};

private:
	class PollAction : public Cancellable {
		EventPoll *poll_;
		Type type_;
		int fd_;
	public:
		PollAction(EventPoll *poll, const Type& type, int fd)
		: poll_(poll),
		  type_(type),
		  fd_(fd)
		{ }

		~PollAction()
		{ }

		void cancel(void)
		{
			poll_->cancel(type_, fd_);
		}
	};

	struct PollHandler {
		EventCallback *callback_;
		Action *action_;

		PollHandler(void)
		: callback_(NULL),
		  action_(NULL)
		{ }

		~PollHandler()
		{
			ASSERT("/event/poll/handler", callback_ == NULL);
			ASSERT("/event/poll/handler", action_ == NULL);
		}

		void callback(Event);
		void cancel(void);
	};
	typedef std::map<int, PollHandler> poll_handler_map_t;

	LogHandle log_;
	poll_handler_map_t read_poll_;
	poll_handler_map_t write_poll_;
	EventPollState *state_;

	EventPoll(void);
	~EventPoll();

	Action *poll(const Type&, int, EventCallback *);
	void cancel(const Type&, int);
	void wait(int);

	bool idle(void) const
	{
		return (read_poll_.empty() && write_poll_.empty());
	}

	void poll(void)
	{
		wait(0);
	}

	void wait(void)
	{
		wait(-1);
	}
};

#endif /* !EVENT_EVENT_POLL_H */
