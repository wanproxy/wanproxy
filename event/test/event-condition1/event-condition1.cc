#include <common/test.h>

#include <event/event_callback.h>
#include <event/event_condition.h>
#include <event/event_main.h>

struct ConditionTest {
	unsigned key_;
	TestGroup& group_;
	Test test_;
	EventConditionVariable cv_;
	Action *action_;

	ConditionTest(unsigned key, TestGroup& group)
	: key_(key),
	  group_(group),
	  test_(group_, "Condition is triggered."),
	  cv_(),
	  action_(NULL)
	{
		EventCallback *cb = callback(this, &ConditionTest::condition_signalled);
		action_ = cv_.wait(cb);
		cv_.signal(Event::Done);
	}

	~ConditionTest()
	{
		{
			Test _(group_, "No outstanding action.");
			if (action_ == NULL)
				_.pass();
		}
	}

	void condition_signalled(Event e)
	{
		test_.pass();
		{
			Test _(group_, "Outstanding action.");
			if (action_ != NULL) {
				action_->cancel();
				action_ = NULL;

				_.pass();
			}
		}
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
	TestGroup g("/test/event/condition1", "EventCondition #1");

	unsigned i;
	for (i = 0; i < 100; i++) {
		new ConditionTest(i, g);
	}

	event_main();
}
