#include <event/callback_handler.h>
#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#define	TIMER_MS	1000

class CallbackHandlerSpeed {
	uintmax_t callback_count_;
	CallbackHandler handler_;
	Action *timeout_action_;
public:
	CallbackHandlerSpeed(void)
	: callback_count_(0),
	  handler_(),
	  timeout_action_(NULL)
	{
		handler_.handler(this, &CallbackHandlerSpeed::callback_complete);
		handler_.wait(handler_.callback()->schedule());

		INFO("/example/callback/handler/speed1") << "Arming timer.";
		timeout_action_ = EventSystem::instance()->timeout(TIMER_MS, callback(this, &CallbackHandlerSpeed::timer));
	}

	~CallbackHandlerSpeed()
	{
		ASSERT("/example/callback/handler/speed1", timeout_action_ == NULL);
	}

private:
	void callback_complete(void)
	{
		callback_count_++;

		handler_.wait(handler_.callback()->schedule());
	}

	void timer(void)
	{
		timeout_action_->cancel();
		timeout_action_ = NULL;

		handler_.cancel();

		INFO("/example/callback/handler/speed1") << "Timer expired; " << callback_count_ << " callbacks.";
	}
};

int
main(void)
{
	INFO("/example/callback/handler/speed1") << "Timer delay: " << TIMER_MS << "ms";

	CallbackHandlerSpeed *cs = new CallbackHandlerSpeed();

	event_main();

	delete cs;
}
