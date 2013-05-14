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

#include <common/test.h>

#include <event/event_callback.h>
#include <event/event_condition.h>
#include <event/event_handler.h>
#include <event/event_main.h>
#include <event/event_system.h>

namespace {
	static unsigned outstanding;
}

struct HandlerTest {
	unsigned key_;
	TestGroup& group_;
	Test test_;
	EventConditionVariable cv_;
	EventHandler handler_;

	HandlerTest(unsigned key, TestGroup& group)
	: key_(key),
	  group_(group),
	  test_(group_, "Handler is triggered."),
	  cv_(),
	  handler_()
	{
		handler_.handler(Event::Done, this, &HandlerTest::condition_signalled);
		handler_.wait(cv_.wait(handler_.callback()));
		cv_.signal(Event::Done);
	}

	~HandlerTest()
	{ }

	void condition_signalled(Event e)
	{
		test_.pass();
		{
			Test _(group_, "Correct event type.");
			if (e.type_ == Event::Done)
				_.pass();
		}

		if (outstanding-- == 1)
			EventSystem::instance()->stop();

		delete this;
	}
};

int
main(void)
{
	TestGroup g("/test/event/handler1", "EventHandler #1");

	outstanding = 100;

	unsigned i;
	for (i = 0; i < 100; i++) {
		new HandlerTest(i, g);
	}

	event_main();
}
