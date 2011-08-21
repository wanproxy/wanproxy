#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>
#include <event/speed_test.h>

static uint8_t zbuf[8192];

class BufferSpeed : SpeedTest {
	uintmax_t bytes_;
public:
	BufferSpeed(void)
	: bytes_(0)
	{
		INFO("/example/buffer/append/speed1") << "Arming timer.";
	}

	~BufferSpeed()
	{ }

private:
	void perform(void)
	{
		Buffer tmp;
		tmp.append(zbuf, sizeof zbuf);
		bytes_ += tmp.length();
	}

	void finish(void)
	{
		INFO("/example/buffer/append/speed1") << "Timer expired; " << bytes_ << " bytes appended.";
	}
};

int
main(void)
{
	memset(zbuf, 0, sizeof zbuf);

	BufferSpeed *cs = new BufferSpeed();

	event_main();

	delete cs;
}
