#include <common/test.h>

#include <tcache/tcache.h>

#define	BACKEND_BYTES	256
#define	BACKEND_BSIZE	8

class TestBackend : public TCacheBackend<uint8_t, BACKEND_BSIZE> {
	TestGroup& group_;
	uint8_t data_[BACKEND_BYTES];
public:
	TestBackend(TestGroup& group)
	: group_(group),
	  data_()
	{
		unsigned i;

		for (i = 0; i < sizeof data_; i++)
			data_[i] = i;
	}

	~TestBackend()
	{
		Test _(group_, "Data has been verified.");

		unsigned i;

		for (i = 0; i < sizeof data_; i++)
			if (data_[i] != (i / 2) + 1)
				break;
		if (i == sizeof data_)
			_.pass();
	}

	bool read(uint8_t byte, uint8_t *dst)
	{
		unsigned i;

		for (i = 0; i < BACKEND_BSIZE; i++)
			*dst++ = data_[byte++];
		return (true);
	}

	bool write(uint8_t byte, const uint8_t *src)
	{
		unsigned i;

		for (i = 0; i < BACKEND_BSIZE; i++)
			data_[byte++] = *src++;
		return (true);
	}
};

int
main(void)
{
	TestGroup group("/test/tcache/test1", "#1");
	{
		TestBackend backend(group);
		TCache<uint8_t, 16, 4, BACKEND_BSIZE> cache(&backend);

		unsigned i;
		for (i = 0; i < BACKEND_BYTES; i++) {
			uint8_t byte;

			{
				Test _(group, "Read success.");
				if (cache.read(i, &byte, 1))
					_.pass();
			}
			{
				Test _(group, "Write success.");
				byte = (byte / 2) + 1;
				if (cache.write(i, &byte, 1))
					_.pass();
			}
		}

		{
			Test _(group, "Flush success.");
			if (cache.flush())
				_.pass();
		}
	}
}
