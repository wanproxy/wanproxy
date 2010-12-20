#include <common/buffer.h>
#include <common/test.h>

int
main(void)
{
	TestGroup g("/test/buffer/append1", "Buffer::append #1");

	Buffer alphabet("abcdefghijklmnopqrstuvwxyz");
	Buffer abcabc;
	abcabc.append(alphabet, 3);
	abcabc.append(alphabet, 3);
	{
		Test _(g, "Check for expected value.");
		if (abcabc.equal("abcabc"))
			_.pass();
	}
}
