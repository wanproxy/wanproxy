#ifndef	TIMEOUT_QUEUE_H
#define	TIMEOUT_QUEUE_H

#include <map>

#include <common/time/time.h>

class TimeoutQueue {
	typedef std::map<NanoTime, CallbackQueue> timeout_map_t;

	LogHandle log_;
	timeout_map_t timeout_queue_;
public:
	TimeoutQueue(void)
	: log_("/event/timeout/queue"),
	  timeout_queue_()
	{ }

	~TimeoutQueue()
	{ }

	bool empty(void) const
	{
		timeout_map_t::const_iterator it;

		/*
		 * Since we allow elements within each CallbackQueue to be
		 * cancelled, we must scan them.
		 *
		 * XXX
		 * We really shouldn't allow this, even if it means we have
		 * to specialize CallbackQueue for this purpose or add
		 * virtual methods to it.  As it is, we can return true
		 * for empty and for ready at the same time.  And in those
		 * cases we have to call perform to garbage collect the
		 * unused CallbackQueues.  We'll, quite conveniently,
		 * never make that call.  Yikes.
		 */
		for (it = timeout_queue_.begin(); it != timeout_queue_.end(); ++it) {
			if (it->second.empty())
				continue;
			return (false);
		}
		return (true);
	}

	Action *append(uintmax_t, SimpleCallback *);
	uintmax_t interval(void) const;
	void perform(void);
	bool ready(void) const;
};

#endif /* !TIMEOUT_QUEUE_H */
