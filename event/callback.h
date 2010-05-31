#ifndef	CALLBACK_H
#define	CALLBACK_H

#include <deque>

class Callback {
protected:
	Callback(void)
	{ }

public:
	virtual ~Callback()
	{ }

protected:
	virtual void operator() (void) = 0;

public:
	void execute(void)
	{
		(*this)();
	}
};

template<class C>
class ObjectCallback : public Callback {
public:
	typedef void (C::*const method_t)(void);

private:
	C *const obj_;
	method_t method_;
public:
	template<typename T>
	ObjectCallback(C *obj, T method)
	: obj_(obj),
	  method_(method)
	{ }

	~ObjectCallback()
	{ }

private:
	void operator() (void)
	{
		(obj_->*method_)();
	}
};

class CallbackQueue {
	class CallbackAction : public Cancellable {
	public:
		CallbackQueue *const queue_;
		const uint64_t generation_;
		Callback *callback_;

		CallbackAction(CallbackQueue *queue, uint64_t generation, Callback *callback)
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
		ASSERT(queue_.empty());
	}

	Action *append(Callback *cb)
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
			ASSERT(a->queue_ == this);
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
		ASSERT(a->queue_ == this);
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
		NOTREACHED();
	}
};

template<class C>
Callback *callback(C *obj, typename ObjectCallback<C>::method_t method)
{
	Callback *cb = new ObjectCallback<C>(obj, method);
	return (cb);
}

#endif /* !CALLBACK_H */
