#include <common/test.h>

#include <event/action.h>

struct Cancelled {
	Test test_;
	bool cancelled_;

	Cancelled(TestGroup& g)
	: test_(g, "Cancellation does occur."),
	  cancelled_(false)
	{
		Action *a = cancellation(this, &Cancelled::cancel);
		a->cancel();
	}

	~Cancelled()
	{
		if (cancelled_)
			test_.pass();
	}

	void cancel(void)
	{
		ASSERT("/cancelled", !cancelled_);
		cancelled_ = true;
	}
};

struct NotCancelled {
	Test test_;
	bool cancelled_;
	Action *action_;

	NotCancelled(TestGroup& g)
	: test_(g, "Cancellation does not occur."),
	  cancelled_(false),
	  action_(NULL)
	{
		action_ = cancellation(this, &NotCancelled::cancel);
	}

	~NotCancelled()
	{
		if (!cancelled_) {
			if (action_ != NULL) {
				action_->cancel();
				action_ = NULL;
				ASSERT("/not/cancelled", cancelled_);
			}
		}
	}

	void cancel(void)
	{
		ASSERT("/not/cancelled", !cancelled_);
		cancelled_ = true;
		test_.pass();
	}
};

int
main(void)
{
	TestGroup g("/test/action/cancel1", "Action::cancel #1");
	{
		Cancelled _(g);
	}
	{
		NotCancelled _(g);
	}
}
