#ifndef	EVENT_ACTION_H
#define	EVENT_ACTION_H

class Action {
	bool cancelled_;
protected:
	Action(void)
	: cancelled_(false)
	{ }

	virtual ~Action()
	{
		ASSERT("/action", cancelled_);
	}

private:
	virtual void do_cancel(void) = 0;

public:
	void cancel(void)
	{
		ASSERT("/action", !cancelled_);
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

template<class C, typename A>
class CancellationArg : public Cancellable {
	typedef void (C::*const method_t)(A);

	C *const obj_;
	method_t method_;
	A arg_;
public:
	template<typename T>
	CancellationArg(C *obj, T method, A arg)
	: obj_(obj),
	  method_(method),
	  arg_(arg)
	{ }

	~CancellationArg()
	{ }

private:
	void cancel(void)
	{
		(obj_->*method_)(arg_);
	}
};

template<class C, typename T>
Action *cancellation(C *obj, T method)
{
	Action *a = new Cancellation<C>(obj, method);
	return (a);
}

template<class C, typename T, typename A>
Action *cancellation(C *obj, T method, A arg)
{
	Action *a = new CancellationArg<C, A>(obj, method, arg);
	return (a);
}

#endif /* !EVENT_ACTION_H */
