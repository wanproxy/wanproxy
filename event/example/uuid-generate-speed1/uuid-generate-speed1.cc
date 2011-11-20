#include <common/buffer.h>

#include <common/uuid/uuid.h>

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>
#include <event/speed_test.h>

class UUIDGenerateSpeed : SpeedTest {
	uintmax_t uuids_;
public:
	UUIDGenerateSpeed(void)
	: uuids_(0)
	{
		perform();
	}

	~UUIDGenerateSpeed()
	{ }

private:
	void perform(void)
	{
		UUID uuid;
		uuid.generate();
		uuids_++;

		schedule();
	}

	void finish(void)
	{
		INFO("/example/uuid/generate/speed1") << "Timer expired; " << uuids_ << " UUIDs generated.";
	}
};

int
main(void)
{
	UUIDGenerateSpeed *cs = new UUIDGenerateSpeed();

	event_main();

	delete cs;
}
