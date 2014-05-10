/*
 * Copyright (c) 2012-2013 Juli Mallett. All rights reserved.
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

#include <event/callback_handler.h>
#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#define	TIMER_MS	1000

class CallbackHandlerSpeed {
	uintmax_t callback_count_;
	CallbackHandler handler_;
	Action *timeout_action_;
public:
	CallbackHandlerSpeed(void)
	: callback_count_(0),
	  handler_(),
	  timeout_action_(NULL)
	{
		handler_.handler(this, &CallbackHandlerSpeed::callback_complete);
		handler_.wait(handler_.callback()->schedule());

		INFO("/example/callback/handler/speed1") << "Arming timer.";
		timeout_action_ = EventSystem::instance()->timeout(TIMER_MS, callback(this, &CallbackHandlerSpeed::timer));
	}

	~CallbackHandlerSpeed()
	{
		ASSERT("/example/callback/handler/speed1", timeout_action_ == NULL);
	}

private:
	void callback_complete(void)
	{
		callback_count_++;

		handler_.wait(handler_.callback()->schedule());
	}

	void timer(void)
	{
		timeout_action_->cancel();
		timeout_action_ = NULL;

		handler_.cancel();

		INFO("/example/callback/handler/speed1") << "Timer expired; " << callback_count_ << " callbacks.";

		EventSystem::instance()->stop();
	}
};

int
main(void)
{
	INFO("/example/callback/handler/speed1") << "Timer delay: " << TIMER_MS << "ms";

	CallbackHandlerSpeed *cs = new CallbackHandlerSpeed();

	event_main();

	delete cs;
}
