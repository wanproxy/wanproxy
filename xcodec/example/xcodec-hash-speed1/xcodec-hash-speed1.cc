#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>
#include <event/speed_test.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_hash.h>

static uint8_t zbuf[XCODEC_SEGMENT_LENGTH * 32];

class XCodecHashSpeed : SpeedTest {
	uintmax_t bytes_;
	uint64_t hash_;
public:
	XCodecHashSpeed(void)
	: bytes_(0),
	  hash_(0)
	{
		perform();
	}

	~XCodecHashSpeed()
	{ }

private:
	void perform(void)
	{
		unsigned i;

		for (i = 0; i < sizeof zbuf; i += XCODEC_SEGMENT_LENGTH)
			hash_ += XCodecHash::hash(zbuf + i);
		zbuf[i] = hash_; /* So the compiler [hopefully] won't optimize out any iterations.  */

		bytes_ += sizeof zbuf;

		schedule();
	}

	void finish(void)
	{
		INFO("/example/xcodec/hash/speed1") << "Timer expired; " << bytes_ << " bytes hashed (final hash " << hash_ << ").";
	}
};

int
main(void)
{
	unsigned i;
	for (i = 0; i < sizeof zbuf; i++)
		zbuf[i] = random();

	XCodecHashSpeed *cs = new XCodecHashSpeed();

	event_main();

	delete cs;
}
