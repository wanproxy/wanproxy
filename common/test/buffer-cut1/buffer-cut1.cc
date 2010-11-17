#include <common/buffer.h>
#include <common/test.h>

#define	FILL_BYTE	(0xa5)
#define	FILL_NUMBER	(1024)
#define	FILL_LENGTH	(242)

int
main(void)
{
	TestGroup g("/test/buffer/cut1", "Buffer::cut #1");

	Buffer big;

	big.append("Hello, ");
	size_t headerlen = big.length();

	uint8_t fill[FILL_LENGTH];
	unsigned i;
	for (i = 0; i < sizeof fill; i++) {
		fill[i] = FILL_BYTE;
	}

	for (i = 0; i < FILL_NUMBER; i++) {
		big.append(fill, sizeof fill);
	}

	big.append("world!\n");

	{
		Test _(g, "Cut fill bytes.");

		Buffer small(big);
		small.cut(headerlen, FILL_NUMBER * FILL_LENGTH);
		if (small.equal("Hello, world!\n"))
			_.pass();
	}

	{
		Test _(g, "Proxy skip and trim.");

		big.cut(0, headerlen);
		big.cut(FILL_NUMBER * FILL_LENGTH, big.length() - (FILL_NUMBER * FILL_LENGTH));
		if (big.length() == FILL_NUMBER * FILL_LENGTH)
			_.pass();
	}

	{
		Test _(g, "Correct fill byte checksum.");

		uint64_t sum = 0;
		while (!big.empty()) {
			sum += big.peek();
			big.skip(1);
		}

		sum /= FILL_LENGTH * FILL_NUMBER * FILL_BYTE;
		if (sum == 1)
			_.pass();
	}

	return (0);
}
