#ifndef	TYPED_CALLBACK_H
#define	TYPED_CALLBACK_H

template<typename T>
class TypedCallback : public Callback {
	T param_;
protected:
	TypedCallback(void)
	: param_()
	{ }

public:
	virtual ~TypedCallback()
	{ }

protected:
	virtual void operator() (T) = 0;

	void operator() (void)
	{
		(*this)(param_);
	}

public:
	void param(T p)
	{
		param_ = p;
	}
};

template<typename T, class C>
class ObjectTypedCallback : public TypedCallback<T> {
public:
	typedef void (C::*const method_t)(T);

private:
	C *const obj_;
	method_t method_;
public:
	template<typename Tm>
	ObjectTypedCallback(C *obj, Tm method)
	: TypedCallback<T>(),
	  obj_(obj),
	  method_(method)
	{ }

	~ObjectTypedCallback()
	{ }

private:
	void operator() (T p)
	{
		(obj_->*method_)(p);
	}
};

template<typename T, class C, typename A>
class ObjectTypedArgCallback : public TypedCallback<T> {
public:
	typedef void (C::*const method_t)(T, A);

private:
	C *const obj_;
	method_t method_;
	A arg_;
public:
	template<typename Tm>
	ObjectTypedArgCallback(C *obj, Tm method, A arg)
	: TypedCallback<T>(),
	  obj_(obj),
	  method_(method),
	  arg_(arg)
	{ }

	~ObjectTypedArgCallback()
	{ }

private:
	void operator() (T p)
	{
		(obj_->*method_)(p, arg_);
	}
};

template<typename T, class C>
TypedCallback<T> *callback(C *obj, void (C::*const method)(T))
{
	TypedCallback<T> *cb = new ObjectTypedCallback<T, C>(obj, method);
	return (cb);
}

template<typename T, class C, typename A>
TypedCallback<T> *callback(C *obj, void (C::*const method)(T, A), A arg)
{
	TypedCallback<T> *cb = new ObjectTypedArgCallback<T, C, A>(obj, method, arg);
	return (cb);
}

#endif /* !TYPED_CALLBACK_H */
