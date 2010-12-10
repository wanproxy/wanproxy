#include <common/buffer.h> /* XXX For class Event */

#include <event/action.h>
#include <event/callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#define	TIMER_MS	1000

class CallbackSpeed {
	uintmax_t callback_count_;
	Action *callback_action_;
	Action *timeout_action_;
public:
	CallbackSpeed(void)
	: callback_count_(0),
	  callback_action_(NULL),
	  timeout_action_(NULL)
	{
		callback_action_ = callback(this, &CallbackSpeed::callback_complete)->schedule();

		INFO("/example/callback/speed1") << "Arming timer.";
		timeout_action_ = EventSystem::instance()->timeout(TIMER_MS, callback(this, &CallbackSpeed::timer));
	}

	~CallbackSpeed()
	{
		ASSERT(timeout_action_ == NULL);
	}

private:
	void callback_complete(void)
	{
		callback_action_->cancel();
		callback_action_ = NULL;

		callback_count_++;

		callback_action_ = callback(this, &CallbackSpeed::callback_complete)->schedule();
	}

	void timer(void)
	{
		timeout_action_->cancel();
		timeout_action_ = NULL;

		ASSERT(callback_action_ != NULL);
		callback_action_->cancel();
		callback_action_ = NULL;

		INFO("/example/callback/speed1") << "Timer expired; " << callback_count_ << " callbacks.";
	}
};

int
main(void)
{
	INFO("/example/callback/speed1") << "Timer delay: " << TIMER_MS << "ms";

	CallbackSpeed *cs = new CallbackSpeed();

	event_main();

	delete cs;
}
