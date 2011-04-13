#include <common/buffer.h>

#include <common/uuid/uuid.h>

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#define	TIMER_MS	1000

class UUIDGenerateSpeed {
	uintmax_t uuids_;
	Action *callback_action_;
	Action *timeout_action_;
public:
	UUIDGenerateSpeed(void)
	: uuids_(0),
	  callback_action_(NULL),
	  timeout_action_(NULL)
	{
		callback_action_ = callback(this, &UUIDGenerateSpeed::callback_complete)->schedule();

		INFO("/example/uuid/generate/speed1") << "Arming timer.";
		timeout_action_ = EventSystem::instance()->timeout(TIMER_MS, callback(this, &UUIDGenerateSpeed::timer));
	}

	~UUIDGenerateSpeed()
	{
		ASSERT(timeout_action_ == NULL);
	}

private:
	void callback_complete(void)
	{
		callback_action_->cancel();
		callback_action_ = NULL;

		UUID uuid;
		uuid.generate();
		uuids_++;

		callback_action_ = callback(this, &UUIDGenerateSpeed::callback_complete)->schedule();
	}

	void timer(void)
	{
		timeout_action_->cancel();
		timeout_action_ = NULL;

		ASSERT(callback_action_ != NULL);
		callback_action_->cancel();
		callback_action_ = NULL;

		INFO("/example/uuid/generate/speed1") << "Timer expired; " << uuids_ << " UUIDs generated.";
	}
};

int
main(void)
{
	INFO("/example/uuid/generate/speed1") << "Timer delay: " << TIMER_MS << "ms";

	UUIDGenerateSpeed *cs = new UUIDGenerateSpeed();

	event_main();

	delete cs;
}
