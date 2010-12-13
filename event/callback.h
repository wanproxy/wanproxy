#ifndef	CALLBACK_H
#define	CALLBACK_H

#include <event/action.h>

class Callback;

class CallbackScheduler {
protected:
	CallbackScheduler(void)
	{ }

public:
	virtual ~CallbackScheduler()
	{ }

	virtual Action *schedule(Callback *) = 0;
};

class Callback {
	CallbackScheduler *scheduler_;
protected:
	Callback(CallbackScheduler *scheduler)
	: scheduler_(scheduler)
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

	Action *schedule(void);
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
	ObjectCallback(CallbackScheduler *scheduler, C *obj, T method)
	: Callback(scheduler),
	  obj_(obj),
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

template<class C, typename A>
class ObjectArgCallback : public Callback {
public:
	typedef void (C::*const method_t)(A);

private:
	C *const obj_;
	method_t method_;
	A arg_;
public:
	template<typename Tm>
	ObjectArgCallback(CallbackScheduler *scheduler, C *obj, Tm method, A arg)
	: Callback(scheduler),
	  obj_(obj),
	  method_(method),
	  arg_(arg)
	{ }

	~ObjectArgCallback()
	{ }

private:
	void operator() (void)
	{
		(obj_->*method_)(arg_);
	}
};

template<class C>
Callback *callback(C *obj, typename ObjectCallback<C>::method_t method)
{
	Callback *cb = new ObjectCallback<C>(NULL, obj, method);
	return (cb);
}

template<class C, typename A>
Callback *callback(C *obj, void (C::*const method)(A), A arg)
{
	Callback *cb = new ObjectArgCallback<C, A>(NULL, obj, method, arg);
	return (cb);
}

#endif /* !CALLBACK_H */
