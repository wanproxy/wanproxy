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
	typedef void (C::*method_t)(void);

private:
	C *obj_;
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
		CallbackQueue *queue_;
		Callback *callback_;

		CallbackAction(CallbackQueue *queue, Callback *callback)
		: Cancellable(),
		  queue_(queue),
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
public:
	CallbackQueue(void)
	: queue_()
	{ }

	~CallbackQueue()
	{ }

	Action *append(Callback *cb)
	{
		CallbackAction *a = new CallbackAction(this, cb);
		queue_.push_back(a);
		return (a);
	}

	bool empty(void)
	{
		return (queue_.empty());
	}

	void perform(void)
	{

		if (queue_.empty())
			return;
		CallbackAction *a = queue_.front();
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
