#include <common/buffer.h> /* XXX For class Event */

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>
#include <event/timeout.h>

#define	CALLBACK_NUMBER	1000
#define	TIMER_MS	1000

class CallbackManySpeed {
	uintmax_t callback_count_;
	Action *callback_action_[CALLBACK_NUMBER];
	Action *timeout_action_;
public:
	CallbackManySpeed(void)
	: callback_count_(0),
	  callback_action_(),
	  timeout_action_(NULL)
	{
		unsigned i;
		for (i = 0; i < CALLBACK_NUMBER; i++)
			callback_action_[i] = EventSystem::instance()->schedule(callback(this, &CallbackManySpeed::callback_complete, i));

		INFO("/example/callback/manyspeed1") << "Arming timer.";
		timeout_action_ = EventSystem::instance()->timeout(TIMER_MS, callback(this, &CallbackManySpeed::timer));
	}

	~CallbackManySpeed()
	{
		ASSERT(timeout_action_ == NULL);
	}

private:
	void callback_complete(unsigned i)
	{
		callback_action_[i]->cancel();
		callback_action_[i] = NULL;

		callback_count_++;

		callback_action_[i] = EventSystem::instance()->schedule(callback(this, &CallbackManySpeed::callback_complete, i));
	}

	void timer(void)
	{
		timeout_action_->cancel();
		timeout_action_ = NULL;

		unsigned i;
		for (i = 0; i < CALLBACK_NUMBER; i++) {
			ASSERT(callback_action_[i] != NULL);
			callback_action_[i]->cancel();
			callback_action_[i] = NULL;
		}

		INFO("/example/callback/manyspeed1") << "Timer expired; " << callback_count_ << " callbacks.";
	}
};

int
main(void)
{
	INFO("/example/callback/manyspeed1") << "Timer delay: " << TIMER_MS << "ms";

	CallbackManySpeed *cs = new CallbackManySpeed();
	EventSystem::instance()->start();
	delete cs;
}
