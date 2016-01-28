/*
 * Copyright (c) 2010-2016 Juli Mallett. All rights reserved.
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
public:
	template<class C>
	class Method;

private:
	bool have_param_;
	std::pair<Ta, Tb> param_;

protected:
	TypedPairCallback(CallbackScheduler *scheduler, Lock *xlock)
	: CallbackBase(scheduler, xlock),
	  have_param_(false),
	  param_()
	{ }

	virtual ~TypedPairCallback()
	{ }

	virtual void operator() (Ta, Tb) = 0;

public:
	void execute(void)
	{
		ASSERT("/typed/callback", have_param_);

		/*
		 * We must do this here so that this callback can
		 * be rescheduled with a new parameter within the
		 * callback itself.
		 */
		std::pair<Ta, Tb> p = param_;
		param_ = std::pair<Ta, Tb>();
		have_param_ = false;

		(*this)(p.first, p.second);
	}

	void param(Ta a, Tb b)
	{
		param_ = std::pair<Ta, Tb>(a, b);
		have_param_ = true;
	}
};

template<typename Ta, typename Tb>
template<class C>
class TypedPairCallback<Ta, Tb>::Method : public TypedPairCallback<Ta, Tb> {
	typedef void (C::*const method_t)(Ta, Tb);

	C *const obj_;
	method_t method_;
public:
	template<typename Tm>
	Method(CallbackScheduler *scheduler, Lock *xlock, C *obj, Tm method)
	: TypedPairCallback<Ta, Tb>(scheduler, xlock),
	  obj_(obj),
	  method_(method)
	{ }

	~Method()
	{ }

private:
	void operator() (Ta a, Tb b)
	{
		(obj_->*method_)(a, b);
	}
};

#endif /* !EVENT_TYPED_PAIR_CALLBACK_H */
