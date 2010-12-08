#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#define	TIMER_MS	1000

static uint8_t zbuf[8192];

class BufferSpeed {
	uintmax_t bytes_;
	Action *callback_action_;
	Action *timeout_action_;
public:
	BufferSpeed(void)
	: callback_action_(NULL),
	  timeout_action_(NULL)
	{
		callback_action_ = callback(this, &BufferSpeed::callback_complete)->schedule();

		INFO("/example/buffer/append/speed1") << "Arming timer.";
		timeout_action_ = EventSystem::instance()->timeout(TIMER_MS, callback(this, &BufferSpeed::timer));
	}

	~BufferSpeed()
	{
		ASSERT(timeout_action_ == NULL);
	}

private:
	void callback_complete(void)
	{
		callback_action_->cancel();
		callback_action_ = NULL;

		Buffer tmp;
		tmp.append(zbuf, sizeof zbuf);
		bytes_ += tmp.length();

		callback_action_ = callback(this, &BufferSpeed::callback_complete)->schedule();
	}

	void timer(void)
	{
		timeout_action_->cancel();
		timeout_action_ = NULL;

		ASSERT(callback_action_ != NULL);
		callback_action_->cancel();
		callback_action_ = NULL;

		INFO("/example/buffer/append/speed1") << "Timer expired; " << bytes_ << " bytes appended.";
	}
};

int
main(void)
{
	INFO("/example/buffer/append/speed1") << "Timer delay: " << TIMER_MS << "ms";

	memset(zbuf, 0, sizeof zbuf);

	BufferSpeed *cs = new BufferSpeed();
	EventSystem::instance()->start();
	delete cs;
}
