#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>
#include <event/speed_test.h>

class CallbackSpeed : SpeedTest {
	uintmax_t callback_count_;
public:
	CallbackSpeed(void)
	: callback_count_(0)
	{
		perform();
	}

	~CallbackSpeed()
	{ }

private:
	void perform(void)
	{
		callback_count_++;

		schedule();
	}

	void finish(void)
	{
		INFO("/example/callback/speed1") << "Timer expired; " << callback_count_ << " callbacks.";
	}
};

int
main(void)
{
	CallbackSpeed *cs = new CallbackSpeed();

	event_main();

	delete cs;
}
