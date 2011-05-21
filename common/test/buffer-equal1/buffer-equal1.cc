#include <common/buffer.h>
#include <common/test.h>

int
main(void)
{
	TestGroup g("/test/buffer/equal1", "Buffer::equal #1");

	// Empty equal tests.
	{
		Buffer empty;
		{
			Test _(g, "Empty Buffer matches itself");
			if (empty.equal(&empty))
				_.pass();
		}
		{
			Buffer buf;
			{
				Test _(g, "Empty Buffer matches empty Buffer");
				if (empty.equal(&buf))
					_.pass();
			}
			{
				Test _(g, "Empty Buffer matches empty Buffer (the other way 'round)");
				if (buf.equal(&empty))
					_.pass();
			}
		}
		{
			std::string str;
			{
				Test _(g, "Empty Buffer matches empty std::string");
				if (empty.equal(str))
					_.pass();
			}
		}
	}
}
