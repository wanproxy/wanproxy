#ifndef	CALLBACK_H
#define	CALLBACK_H

class Action;

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
	ObjectArgCallback(C *obj, Tm method, A arg)
	: Callback(),
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
	Callback *cb = new ObjectCallback<C>(obj, method);
	return (cb);
}

template<class C, typename A>
Callback *callback(C *obj, void (C::*const method)(A), A arg)
{
	Callback *cb = new ObjectArgCallback<C, A>(obj, method, arg);
	return (cb);
}

#endif /* !CALLBACK_H */
