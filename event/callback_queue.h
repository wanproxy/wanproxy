#ifndef	CALLBACK_QUEUE_H
#define	CALLBACK_QUEUE_H

#include <deque>

#include <event/callback.h>

class CallbackQueue : public CallbackScheduler {
	class CallbackAction : public Cancellable {
	public:
		CallbackQueue *const queue_;
		const uint64_t generation_;
		CallbackBase *callback_;

		CallbackAction(CallbackQueue *queue, uint64_t generation, CallbackBase *callback)
		: Cancellable(),
		  queue_(queue),
		  generation_(generation),
		  callback_(callback)
		{ }

		~CallbackAction()
		{
			delete callback_;
			callback_ = NULL;
		}

		void cancel(void)
		{
			queue_->cancel(this);
		}
	};

	friend class CallbackAction;

	std::deque<CallbackAction *> queue_;
	uint64_t generation_;
public:
	CallbackQueue(void)
	: queue_(),
	  generation_(0)
	{ }

	~CallbackQueue()
	{
		ASSERT("/callback/queue", queue_.empty());
	}

	Action *schedule(CallbackBase *cb)
	{
		CallbackAction *a = new CallbackAction(this, generation_, cb);
		queue_.push_back(a);
		return (a);
	}

	/*
	 * Runs all callbacks that have already been queued, but none that
	 * are added by callbacks that are called as part of the drain
	 * operation.  Returns true if there are queued callbacks that were
	 * added during drain.
	 */
	bool drain(void)
	{
		if (queue_.empty())
			return (false);

		generation_++;
		while (!queue_.empty()) {
			CallbackAction *a = queue_.front();
			ASSERT("/callback/queue", a->queue_ == this);
			if (a->generation_ >= generation_)
				return (true);
			a->callback_->execute();
		}
		return (false);
	}

	bool empty(void) const
	{
		return (queue_.empty());
	}

	void perform(void)
	{
		if (queue_.empty())
			return;
		CallbackAction *a = queue_.front();
		ASSERT("/callback/queue", a->queue_ == this);
		a->callback_->execute();
	}

private:
	void cancel(CallbackAction *a)
	{
		std::deque<CallbackAction *>::iterator it;

		for (it = queue_.begin(); it != queue_.end(); ++it) {
			if (*it != a)
				continue;
			queue_.erase(it);
			return;
		}
		NOTREACHED("/callback/queue");
	}
};

#endif /* !CALLBACK_QUEUE_H */
