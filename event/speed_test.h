/*
 * Copyright (c) 2011 Juli Mallett. All rights reserved.
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

#ifndef	EVENT_SPEED_TEST_H
#define	EVENT_SPEED_TEST_H

#include <event/event_callback.h>
#include <event/event_system.h>

#define	SPEED_TEST_TIMER_MS	1000

class SpeedTest {
	Action *callback_action_;
	Action *timeout_action_;
public:
	SpeedTest(void)
	: callback_action_(NULL),
	  timeout_action_(NULL)
	{
		timeout_action_ = EventSystem::instance()->timeout(SPEED_TEST_TIMER_MS, callback(this, &SpeedTest::timer));
	}

	virtual ~SpeedTest()
	{
		ASSERT("/speed/test", callback_action_ == NULL);
		ASSERT("/speed/test", timeout_action_ == NULL);
	}

private:
	void callback_complete(void)
	{
		callback_action_->cancel();
		callback_action_ = NULL;

		perform();
	}

	void timer(void)
	{
		timeout_action_->cancel();
		timeout_action_ = NULL;

		if (callback_action_ != NULL) {
			callback_action_->cancel();
			callback_action_ = NULL;
		}

		finish();
	}

protected:
	void schedule(void)
	{
		ASSERT("/speed/test", callback_action_ == NULL);
		callback_action_ = callback(this, &SpeedTest::callback_complete)->schedule();
	}

	virtual void perform(void)
	{ }

	virtual void finish(void)
	{ }
};

#endif /* !EVENT_SPEED_TEST_H */
