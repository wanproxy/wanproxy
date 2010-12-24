#include <common/buffer.h>
#include <common/test.h>

#define	FILL_BYTE	(0xa5)
#define	FILL_NUMBER	(1024)
#define	FILL_LENGTH	(242)

int
main(void)
{
	TestGroup g("/test/buffer/cut1", "Buffer::cut #1");

	unsigned n;
	for (n = 1; n < 23; n++) {
		Buffer big;

		big.append("Hello, ");
		size_t headerlen = big.length();

		uint8_t fill[n * FILL_LENGTH];
		unsigned i;
		for (i = 0; i < sizeof fill; i++) {
			fill[i] = FILL_BYTE;
		}

		for (i = 0; i < FILL_NUMBER; i++) {
			big.append(fill, sizeof fill);
		}

		big.append("world!\n");

		{
			Buffer clip;
			Buffer small(big);
			small.cut(headerlen, FILL_NUMBER * sizeof fill, &clip);
			{ 
				Test _(g, "Cut fill bytes.");
				if (small.equal("Hello, world!\n"))
					_.pass();
			}
			{
				Test _(g, "Expected clipped fill bytes.");
				Buffer expected;
				for (i = 0; i < FILL_NUMBER; i++) {
					expected.append(fill, sizeof fill);
				}
				if (clip.equal(&expected))
					_.pass();
			}
		}

		{
			Buffer clip;
			big.cut(0, headerlen, &clip);
			big.cut(FILL_NUMBER * sizeof fill,
				big.length() - (FILL_NUMBER * sizeof fill), &clip);
			{
				Test _(g, "Proxy skip and trim.");
				if (big.length() == FILL_NUMBER * sizeof fill)
					_.pass();
			}
			{
				Test _(g, "Expected clipped beginning and end.");
				if (clip.equal("Hello, world!\n"))
					_.pass();
			}
		}

		{
			Test _(g, "Correct fill byte checksum.");

			uint64_t sum = 0;
			Buffer::SegmentIterator iter = big.segments();
			while (!iter.end()) {
				const BufferSegment *seg = *iter;
				const uint8_t *p;
				const uint8_t *q = seg->end();
				for (p = seg->data(); p < q; p++)
					sum += *p;
				iter.next();
			}
			big.clear();

			sum /= FILL_NUMBER * sizeof fill * FILL_BYTE;
			if (sum == 1)
				_.pass();
		}
	}

	return (0);
}
