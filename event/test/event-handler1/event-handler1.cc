#include <common/test.h>

#include <event/event_callback.h>
#include <event/event_condition.h>
#include <event/event_handler.h>
#include <event/event_main.h>

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

		delete this;
	}
};

int
main(void)
{
	TestGroup g("/test/event/handler1", "EventHandler #1");

	unsigned i;
	for (i = 0; i < 100; i++) {
		new HandlerTest(i, g);
	}

	event_main();
}
