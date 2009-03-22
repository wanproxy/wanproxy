#ifndef	EVENT_POLL_H
#define	EVENT_POLL_H

#include <map>

class EventPoll {
	friend class EventSystem;
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
			ASSERT(callback_ == NULL);
			ASSERT(action_ == NULL);
		}

		void callback(Event);
		void cancel(void);
	};
	typedef std::map<int, PollHandler> poll_handler_map_t;

	LogHandle log_;
	poll_handler_map_t read_poll_;
	poll_handler_map_t write_poll_;
#if defined(USE_POLL_KQUEUE)
	int kq_;
#elif defined(USE_POLL_POLL)
#elif defined(USE_POLL_SELECT)
#else
#error "Unsupported poll mechanism."
#endif

	EventPoll(void);
	~EventPoll();

	Action *poll(const Type&, int, EventCallback *);
	void cancel(const Type&, int);

	bool idle(void) const;
	void poll(void);
	void wait(int);
	void wait(void);
};

#endif /* !EVENT_POLL_H */
