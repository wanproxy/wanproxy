#ifndef	SPEED_TEST_H
#define	SPEED_TEST_H

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

#endif /* !SPEED_TEST_H */
