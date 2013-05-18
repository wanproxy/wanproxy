/*
 * Copyright (c) 2009-2013 Juli Mallett. All rights reserved.
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

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#define	TIMER1_MS	100
#define	TIMER2_MS	2000

class TimeoutTest {
	Action *action_;
public:
	TimeoutTest(void)
	: action_(NULL)
	{
		INFO("/example/timeout/test1") << "Arming timer 1.";
		action_ = EventSystem::instance()->timeout(TIMER1_MS, callback(this, &TimeoutTest::timer1));
	}

	~TimeoutTest()
	{
		ASSERT("/example/timeout/test1", action_ == NULL);
	}

private:
	void timer1(void)
	{
		action_->cancel();
		action_ = NULL;

		INFO("/example/timeout/test1") << "Timer 1 expired, arming timer 2.";

		action_ = EventSystem::instance()->timeout(TIMER2_MS, callback(this, &TimeoutTest::timer2));
	}

	void timer2(void)
	{
		action_->cancel();
		action_ = NULL;

		INFO("/example/timeout/test1") << "Timer 2 expired.";

		EventSystem::instance()->stop();
	}
};

int
main(void)
{
	INFO("/example/timeout/test1") << "Timer 1 delay: " << TIMER1_MS;
	INFO("/example/timeout/test1") << "Timer 2 delay: " << TIMER2_MS;

	TimeoutTest *tt = new TimeoutTest();

	event_main();

	delete tt;
}
