#include <common/buffer.h>
#include <common/test.h>

int
main(void)
{
	TestGroup g("/test/buffer/pullup2", "BufferSegment::pullup #2");

	uint8_t data[BUFFER_SEGMENT_SIZE / 2];
	unsigned i;

	for (i = 0; i < sizeof data; i++)
		data[i] = random() & 0xff;

	{

		BufferSegment *seg = BufferSegment::create();
		while (seg->avail() > 0) {
			BufferSegment *seg2 = seg->append("A");
			Test _(g, "Byte append");
			if (seg2 == seg)
				_.pass();
		}
		{
			Test _(g, "Skip several");
			BufferSegment *seg2 = seg->skip(sizeof data);
			if (seg2 == seg)
				_.pass();
		}
		{
			Test _(g, "Append with pullup required");
			BufferSegment *seg2 = seg->append(data, sizeof data);
			if (seg2 == seg)
				_.pass();
		}
		{
			uint8_t check[BUFFER_SEGMENT_SIZE];

			for (i = 0; i < sizeof data; i++)
				check[i] = 'A';
			for (i = 0; i < sizeof data; i++)
				check[i + sizeof data] = data[i];

			Test _(g, "Check data contents");
			if (seg->equal(check, sizeof check))
				_.pass();
		}
		seg->unref();
	}
}
