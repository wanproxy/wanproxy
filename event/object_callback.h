/*
 * Copyright (c) 2008-2013 Juli Mallett. All rights reserved.
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

#ifndef	EVENT_OBJECT_CALLBACK_H
#define	EVENT_OBJECT_CALLBACK_H

#include <common/thread/lock.h>

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
	ObjectMethodCallback(CallbackScheduler *scheduler, Lock *xlock, C *obj, T method)
	: SimpleCallback(scheduler, xlock),
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
	ObjectMethodArgCallback(CallbackScheduler *scheduler, Lock *xlock, C *obj, Tm method, A arg)
	: SimpleCallback(scheduler, xlock),
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
SimpleCallback *callback(Lock *lock, C *obj, typename ObjectMethodCallback<C>::method_t method)
{
	ASSERT_LOCK_OWNED("/callback/simple", lock);
	SimpleCallback *cb = new ObjectMethodCallback<C>(NULL, lock, obj, method);
	return (cb);
}

template<class C, typename A>
SimpleCallback *callback(Lock *lock, C *obj, void (C::*const method)(A), A arg)
{
	ASSERT_LOCK_OWNED("/callback/simple", lock);
	SimpleCallback *cb = new ObjectMethodArgCallback<C, A>(NULL, lock, obj, method, arg);
	return (cb);
}

template<class C>
SimpleCallback *callback(CallbackScheduler *scheduler, Lock *lock, C *obj, typename ObjectMethodCallback<C>::method_t method)
{
	ASSERT_LOCK_OWNED("/callback/simple", lock);
	SimpleCallback *cb = new ObjectMethodCallback<C>(scheduler, lock, obj, method);
	return (cb);
}

template<class C, typename A>
SimpleCallback *callback(CallbackScheduler *scheduler, Lock *lock, C *obj, void (C::*const method)(A), A arg)
{
	ASSERT_LOCK_OWNED("/callback/simple", lock);
	SimpleCallback *cb = new ObjectMethodArgCallback<C, A>(scheduler, lock, obj, method, arg);
	return (cb);
}

#endif /* !EVENT_OBJECT_CALLBACK_H */
