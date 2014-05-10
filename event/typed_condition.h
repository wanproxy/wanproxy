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

#ifndef	EVENT_TYPED_CONDITION_H
#define	EVENT_TYPED_CONDITION_H

#include <event/condition.h>
#include <event/typed_callback.h>

template<typename T>
class TypedCondition {
protected:
	TypedCondition(void)
	{ }

	virtual ~TypedCondition()
	{ }

public:
	virtual void signal(T) = 0;
	virtual Action *wait(TypedCallback<T> *) = 0;
};

template<typename T>
class TypedConditionVariable : public TypedCondition<T> {
	Action *wait_action_;
	TypedCallback<T> *wait_callback_;
public:
	TypedConditionVariable(void)
	: wait_action_(NULL),
	  wait_callback_(NULL)
	{ }

	~TypedConditionVariable()
	{
		ASSERT("/typed/condition/variable", wait_action_ == NULL);
		ASSERT("/typed/condition/variable", wait_callback_ == NULL);
	}

	void signal(T p)
	{
		if (wait_callback_ == NULL)
			return;
		ASSERT("/typed/condition/variable", wait_action_ == NULL);
		wait_callback_->param(p);
		wait_action_ = wait_callback_->schedule();
		wait_callback_ = NULL;
	}

	Action *wait(TypedCallback<T> *cb)
	{
		ASSERT("/typed/condition/variable", wait_action_ == NULL);
		ASSERT("/typed/condition/variable", wait_callback_ == NULL);

		wait_callback_ = cb;

		return (cancellation(this, &TypedConditionVariable::wait_cancel));
	}

private:
	void wait_cancel(void)
	{
		if (wait_callback_ != NULL) {
			ASSERT("/typed/condition/variable", wait_action_ == NULL);

			delete wait_callback_;
			wait_callback_ = NULL;
		} else {
			ASSERT("/typed/condition/variable", wait_action_ != NULL);

			wait_action_->cancel();
			wait_action_ = NULL;
		}
	}
};

#endif /* !EVENT_TYPED_CONDITION_H */
