#include <common/buffer.h>
#include <common/test.h>

static uint8_t long_prefix[8192];

int
main(void)
{
	TestGroup g("/test/buffer/prefix1", "Buffer::prefix #1");

	srandom(1318);

	unsigned i;
	for (i = 0; i < sizeof long_prefix; i++) {
		long_prefix[i] = random() % 17;
		if (i == 0 && long_prefix[i] == 'A')
			long_prefix[i] = '>';
	}

	// Long prefix tests.
	{
		Buffer pfx(long_prefix, sizeof long_prefix);
		{
			Buffer buf(long_prefix, sizeof long_prefix);
			buf.append(std::string("ABCD"));
			{
				Test _(g, "Long prefix matched by Buffer");
				if (buf.prefix(&pfx))
					_.pass();
			}
			{
				Test _(g, "Long prefix matched by array");
				if (buf.prefix(long_prefix, sizeof long_prefix))
					_.pass();
			}
		}
		{
			Buffer buf;
			buf.append(std::string("ABCD"));
			buf.append(long_prefix, sizeof long_prefix);
			{
				Test _(g, "Long prefix not matched by Buffer with leading noise");
				if (!buf.prefix(&pfx))
					_.pass();
			}
			{
				Test _(g, "Long prefix not matched by array with leading noise");
				if (!buf.prefix(long_prefix,
						sizeof long_prefix))
					_.pass();
			}
			buf.skip(4);
			{
				Test _(g, "Long prefix matched by Buffer with skipped leading noise");
				if (buf.prefix(&pfx))
					_.pass();
			}
			{
				Test _(g, "Long prefix matched by array with skipped leading noise");

				if (buf.prefix(long_prefix, sizeof long_prefix))
					_.pass();
			}
		}
	}

	// Short prefix tests.
	{
		Buffer buf("ABCD");
		{
			Buffer pfx("ABC");
			Test _(g, "Short prefix matched");
			if (buf.prefix(&pfx))
				_.pass();
		}
		{
			Buffer pfx("BCD");
			Test _(g, "Short non-present prefix not matched");
			if (!buf.prefix(&pfx))
				_.pass();
		}
	}
}
