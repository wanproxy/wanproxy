#include <common/buffer.h> /* XXX For class Event */

#include <event/action.h>
#include <event/callback.h>
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
		ASSERT(action_ == NULL);
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
