#ifndef	OBJECT_CALLBACK_H
#define	OBJECT_CALLBACK_H

#include <event/callback.h>

template<class C>
class ObjectMethodCallback : public SimpleCallback {
public:
	typedef void (C::*const method_t)(void);

private:
	C *const obj_;
	method_t method_;
public:
	template<typename T>
	ObjectMethodCallback(CallbackScheduler *scheduler, C *obj, T method)
	: SimpleCallback(scheduler),
	  obj_(obj),
	  method_(method)
	{ }

	~ObjectMethodCallback()
	{ }

private:
	void operator() (void)
	{
		(obj_->*method_)();
	}
};

template<class C, typename A>
class ObjectMethodArgCallback : public SimpleCallback {
public:
	typedef void (C::*const method_t)(A);

private:
	C *const obj_;
	method_t method_;
	A arg_;
public:
	template<typename Tm>
	ObjectMethodArgCallback(CallbackScheduler *scheduler, C *obj, Tm method, A arg)
	: SimpleCallback(scheduler),
	  obj_(obj),
	  method_(method),
	  arg_(arg)
	{ }

	~ObjectMethodArgCallback()
	{ }

private:
	void operator() (void)
	{
		(obj_->*method_)(arg_);
	}
};

template<class C>
SimpleCallback *callback(C *obj, typename ObjectMethodCallback<C>::method_t method)
{
	SimpleCallback *cb = new ObjectMethodCallback<C>(NULL, obj, method);
	return (cb);
}

template<class C, typename A>
SimpleCallback *callback(C *obj, void (C::*const method)(A), A arg)
{
	SimpleCallback *cb = new ObjectMethodArgCallback<C, A>(NULL, obj, method, arg);
	return (cb);
}

template<class C>
SimpleCallback *callback(CallbackScheduler *scheduler, C *obj, typename ObjectMethodCallback<C>::method_t method)
{
	SimpleCallback *cb = new ObjectMethodCallback<C>(scheduler, obj, method);
	return (cb);
}

template<class C, typename A>
SimpleCallback *callback(CallbackScheduler *scheduler, C *obj, void (C::*const method)(A), A arg)
{
	SimpleCallback *cb = new ObjectMethodArgCallback<C, A>(scheduler, obj, method, arg);
	return (cb);
}

#endif /* !OBJECT_CALLBACK_H */
