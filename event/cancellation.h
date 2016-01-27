/*
 * Copyright (c) 2008-2016 Juli Mallett. All rights reserved.
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

#ifndef	EVENT_CANCELLATION_H
#define	EVENT_CANCELLATION_H

#include <common/thread/lock.h>

#include <event/action.h>

/*
 * TODO
 * It would be nice to have these cancellations track their state,
 * to help detect programming errors, namely whether they are waiting
 * to be cancelled, have never been used, or have been used and
 * cancelled.
 */

/*
 * This is a persistent cancellation which invokes a method on an
 * object with a predetermined lock held.
 */
template<class C>
class Cancellation : public Action {
	typedef void (C::*const method_t)(void);

	Lock *lock_;
	C *const obj_;
	method_t method_;
public:
	template<typename T>
	Cancellation(Lock *lock, C *obj, T method)
	: lock_(lock),
	  obj_(obj),
	  method_(method)
	{
		ASSERT_LOCK_OWNED("/cancellation", lock_);
	}

	~Cancellation()
	{ }

private:
	void cancel(void)
	{
		ScopedLock _(lock_);
		(obj_->*method_)();
	}
};

#endif /* !EVENT_CANCELLATION_H */
