/*
 * Copyright (c) 2012 Juli Mallett. All rights reserved.
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

#ifndef	EVENT_CALLBACK_HANDLER_H
#define	EVENT_CALLBACK_HANDLER_H

#include <event/action.h>
#include <event/callback.h>
#include <event/object_callback.h>

class CallbackHandler {
	Action *action_;
	SimpleCallback *callback_;
public:
	CallbackHandler(void)
	: action_(NULL),
	  callback_(NULL)
	{ }

	~CallbackHandler()
	{
		if (action_ != NULL) {
			action_->cancel();
			action_ = NULL;
		}

		ASSERT("/callback/handler", callback_ != NULL);
		delete callback_;
		callback_ = NULL;
	}

	SimpleCallback *callback(void)
	{
		ASSERT("/callback/handler", callback_ != NULL);
		return (::callback(this, &CallbackHandler::handle_callback));
	}

	void cancel(void)
	{
		ASSERT("/callback/handler", action_ != NULL);

		action_->cancel();
		action_ = NULL;
	}

	template<typename Tobj, typename Tmethod>
	void handler(Tobj obj, Tmethod method)
	{
		ASSERT("/callback/handler", action_ == NULL);
		ASSERT("/callback/handler", callback_ == NULL);

		callback_ = ::callback(obj, method);
	}

	template<typename Tobj, typename Tmethod, typename Targ>
	void handler(Tobj obj, Tmethod method, Targ arg)
	{
		ASSERT("/callback/handler", action_ == NULL);
		ASSERT("/callback/handler", callback_ == NULL);

		callback_ = ::callback(obj, method, arg);
	}

	void wait(Action *action)
	{
		ASSERT("/callback/handler", action_ == NULL);
		ASSERT("/callback/handler", action != NULL);

		action_ = action;
	}

private:
	void handle_callback(void)
	{
		ASSERT("/callback/handler", action_ != NULL);

		action_->cancel();
		action_ = NULL;

		callback_->execute();

		/*
		 * XXX
		 * This is a great place to hook in for debugging/tracing
		 * and to show whether another action was queued inside the
		 * callback execution path, etc.
		 */
	}
};

#endif /* !EVENT_CALLBACK_HANDLER_H */
