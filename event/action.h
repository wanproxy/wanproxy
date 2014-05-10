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
