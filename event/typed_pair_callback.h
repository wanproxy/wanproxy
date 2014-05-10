/*
 * Copyright (c) 2010-2013 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	EVENT_TYPED_PAIR_CALLBACK_H
#define	EVENT_TYPED_PAIR_CALLBACK_H

#include <event/callback.h>

/*
 * XXX
 * Feels like I can get std::pair and some sort
 * of application template to do the heavy lifting
 * here to avoid duplication of TypedCallback<T>.
 */
template<typename Ta, typename Tb>
class TypedPairCallback : public CallbackBase {
	bool have_param_;
	std::pair<Ta, Tb> param_;
protected:
	TypedPairCallback(CallbackScheduler *scheduler)
	: CallbackBase(scheduler),
	  have_param_(false),
	  param_()
	{ }

public:
	virtual ~TypedPairCallback()
	{ }

protected:
	virtual void operator() (Ta, Tb) = 0;

public:
	void execute(void)
	{
		ASSERT("/typed/pair/callback", have_param_);
		(*this)(param_.first, param_.second);
	}

	void param(Ta a, Tb b)
	{
		param_ = typename std::pair<Ta, Tb>(a, b);
		have_param_ = true;
	}
};

template<typename Ta, typename Tb, class C>
class ObjectTypedPairCallback : public TypedPairCallback<Ta, Tb> {
public:
	typedef void (C::*const method_t)(Ta, Tb);

private:
	C *const obj_;
	method_t method_;
public:
	template<typename Tm>
	ObjectTypedPairCallback(CallbackScheduler *scheduler, C *obj, Tm method)
	: TypedPairCallback<Ta, Tb>(scheduler),
	  obj_(obj),
	  method_(method)
	{ }

	~ObjectTypedPairCallback()
	{ }

private:
	void operator() (Ta a, Tb b)
	{
		(obj_->*method_)(a, b);
	}
};

template<typename Ta, typename Tb, class C, typename A>
class ObjectTypedPairArgCallback : public TypedPairCallback<Ta, Tb> {
public:
	typedef void (C::*const method_t)(Ta, Tb, A);

private:
	C *const obj_;
	method_t method_;
	A arg_;
public:
	template<typename Tm>
	ObjectTypedPairArgCallback(CallbackScheduler *scheduler, C *obj, Tm method, A arg)
	: TypedPairCallback<Ta, Tb>(scheduler),
	  obj_(obj),
	  method_(method),
	  arg_(arg)
	{ }

	~ObjectTypedPairArgCallback()
	{ }

private:
	void operator() (Ta a, Tb b)
	{
		(obj_->*method_)(a, b, arg_);
	}
};

template<typename Ta, typename Tb, class C>
TypedPairCallback<Ta, Tb> *callback(C *obj, void (C::*const method)(Ta, Tb))
{
	TypedPairCallback<Ta, Tb> *cb = new ObjectTypedPairCallback<Ta, Tb, C>(NULL, obj, method);
	return (cb);
}

template<typename Ta, typename Tb, class C, typename A>
TypedPairCallback<Ta, Tb> *callback(C *obj, void (C::*const method)(Ta, Tb, A), A arg)
{
	TypedPairCallback<Ta, Tb> *cb = new ObjectTypedPairArgCallback<Ta, Tb, C, A>(NULL, obj, method, arg);
	return (cb);
}

template<typename Ta, typename Tb, class C>
TypedPairCallback<Ta, Tb> *callback(CallbackScheduler *scheduler, C *obj, void (C::*const method)(Ta, Tb))
{
	TypedPairCallback<Ta, Tb> *cb = new ObjectTypedPairCallback<Ta, Tb, C>(scheduler, obj, method);
	return (cb);
}

template<typename Ta, typename Tb, class C, typename A>
TypedPairCallback<Ta, Tb> *callback(CallbackScheduler *scheduler, C *obj, void (C::*const method)(Ta, Tb, A), A arg)
{
	TypedPairCallback<Ta, Tb> *cb = new ObjectTypedPairArgCallback<Ta, Tb, C, A>(scheduler, obj, method, arg);
	return (cb);
}

#endif /* !EVENT_TYPED_PAIR_CALLBACK_H */
