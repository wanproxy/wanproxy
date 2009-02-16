#ifndef	EVENT_CALLBACK_H
#define	EVENT_CALLBACK_H


class EventCallback : public Callback {
	void *userdata_;
	Event event_;
protected:
	EventCallback(void *userdata)
	: userdata_(userdata),
	  event_(Event::Invalid, 0)
	{ }

public:
	virtual ~EventCallback()
	{ }

protected:
	virtual void operator() (Event, void *) = 0;

	void operator() (void)
	{
		(*this)(event_, userdata_);
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
	typedef void (C::*method_t)(Event, void *);

private:
	C *obj_;
	method_t method_;
public:
	template<typename T>
	ObjectEventCallback(C *obj, T method, void *userdata)
	: EventCallback(userdata),
	  obj_(obj),
	  method_(method)
	{ }

	~ObjectEventCallback()
	{
	}

private:
	void operator() (Event e, void *userdata)
	{
		(obj_->*method_)(e, userdata);
	}
};

template<class C>
EventCallback *callback(C *obj,
			typename ObjectEventCallback<C>::method_t method,
			void *userdata = NULL)
{
	EventCallback *cb = new ObjectEventCallback<C>(obj, method, userdata);
	return (cb);
}

#endif /* !EVENT_CALLBACK_H */
