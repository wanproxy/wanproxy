#ifndef	TIMEOUT_H
#define	TIMEOUT_H

#include <map>

class TimeoutQueue {
	typedef std::map<time_t, CallbackQueue> timeout_map_t;

	timeout_map_t timeout_queue_;
public:
	TimeoutQueue(void)
	: timeout_queue_()
	{ }

	~TimeoutQueue()
	{ }

	bool empty(void) const
	{
		/*
		 * Since we allow elements within each CallbackQueue to be
		 * cancelled, this may be incorrect, but will be corrected by
		 * the perform method.  The same caveat applies to ready() and
		 * interval().  Luckily, it should be seldom that a user is
		 * cancelling a callback that has not been invoked by perform().
		 */
		return (timeout_queue_.empty());
	}

	Action *append(unsigned, Callback *);
	time_t interval(void) const;
	void perform(void);
	bool ready(void) const;
};

#endif /* !TIMEOUT_H */
