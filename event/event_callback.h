#ifndef	EVENT_CALLBACK_H
#define	EVENT_CALLBACK_H

class EventCallback : public Callback {
	Event event_;
protected:
	EventCallback(void)
	: event_(Event::Invalid, 0)
	{ }

public:
	virtual ~EventCallback()
	{ }

protected:
	virtual void operator() (Event) = 0;

	void operator() (void)
	{
		(*this)(event_);
	}

public:
	void event(Event e)
	{
		event_ = e;
	}
};

template<class C>
class ObjectEventCallback : public EventCallback {
public:
	typedef void (C::*const method_t)(Event);

private:
	C *const obj_;
	method_t method_;
public:
	template<typename T>
	ObjectEventCallback(C *obj, T method)
	: EventCallback(),
	  obj_(obj),
	  method_(method)
	{ }

	~ObjectEventCallback()
	{ }

private:
	void operator() (Event e)
	{
		(obj_->*method_)(e);
	}
};

template<class C, typename A>
class ObjectEventArgCallback : public EventCallback {
public:
	typedef void (C::*const method_t)(Event, A);

private:
	C *const obj_;
	method_t method_;
	A arg_;
public:
	template<typename T>
	ObjectEventArgCallback(C *obj, T method, A arg)
	: EventCallback(),
	  obj_(obj),
	  method_(method),
	  arg_(arg)
	{ }

	~ObjectEventArgCallback()
	{ }

private:
	void operator() (Event e)
	{
		(obj_->*method_)(e, arg_);
	}
};

template<class C>
EventCallback *callback(C *obj,
			typename ObjectEventCallback<C>::method_t method)
{
	EventCallback *cb = new ObjectEventCallback<C>(obj, method);
	return (cb);
}

template<class C, typename A>
EventCallback *callback(C *obj,
			typename ObjectEventArgCallback<C, A>::method_t method,
			A arg)
{
	EventCallback *cb = new ObjectEventArgCallback<C, A>(obj, method, arg);
	return (cb);
}

#endif /* !EVENT_CALLBACK_H */
