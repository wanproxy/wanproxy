#ifndef	ACTION_H
#define	ACTION_H

class Action {
	bool cancelled_;
protected:
	Action(void)
	: cancelled_(false)
	{ }

	virtual ~Action()
	{
		ASSERT(cancelled_);
	}

private:
	virtual void do_cancel(void) = 0;

public:
	void cancel(void)
	{
		ASSERT(!cancelled_);
		do_cancel();
		cancelled_ = true;
		delete this;
	}
};

class Cancellable : public Action {
protected:
	Cancellable(void)
	: Action()
	{ }

	virtual ~Cancellable()
	{ }

private:
	void do_cancel(void)
	{
		cancel();
	}

	virtual void cancel(void) = 0;
};

template<class C>
class Cancellation : public Cancellable {
	typedef void (C::*const method_t)(void);

	C *const obj_;
	method_t method_;
public:
	template<typename T>
	Cancellation(C *obj, T method)
	: obj_(obj),
	  method_(method)
	{ }

	~Cancellation()
	{ }

private:
	void cancel(void)
	{
		(obj_->*method_)();
	}
};

template<class C, typename T>
Action *cancellation(C *obj, T method)
{
	Action *a = new Cancellation<C>(obj, method);
	return (a);
}

#endif /* !ACTION_H */
