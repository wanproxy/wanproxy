#include <common/buffer.h>
#include <common/test.h>

class PrefixGenerator {
	unsigned seed_;
public:
	PrefixGenerator(unsigned seed)
	: seed_(seed)
	{ }

	~PrefixGenerator()
	{ }

	Buffer generate(void) const
	{
		srandom(seed_);

		uint8_t long_prefix[8192];

		unsigned i;
		for (i = 0; i < sizeof long_prefix; i++) {
			long_prefix[i] = random() % 17;
			if (i == 0 && long_prefix[i] == 'A')
				long_prefix[i] = '>';
		}
		return (Buffer(long_prefix, sizeof long_prefix));
	}
};

int
main(void)
{
	TestGroup g("/test/buffer/return1", "Buffer::prefix #1/Buffer return");

	PrefixGenerator gen(1318);

	// Long prefix tests.
	{
		Buffer pfx = gen.generate();
		{
			Buffer buf(gen.generate());
			buf.append(std::string("ABCD"));
			{
				Test _(g, "Long prefix matched by Buffer");
				if (buf.prefix(&pfx))
					_.pass();
			}
			{
				Test _(g, "Long prefix matched by array");
				uint8_t long_prefix[8192];
				gen.generate().copyout(long_prefix, sizeof long_prefix);
				if (buf.prefix(long_prefix, sizeof long_prefix))
					_.pass();
			}
		}
		{
			Buffer buf;
			buf.append(std::string("ABCD"));
			buf.append(gen.generate());
			{
				Test _(g, "Long prefix not matched by Buffer with leading noise");
				if (!buf.prefix(&pfx))
					_.pass();
			}
			{
				Test _(g, "Long prefix not matched by array with leading noise");
				uint8_t long_prefix[8192];
				gen.generate().copyout(long_prefix, sizeof long_prefix);
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

				uint8_t long_prefix[8192];
				gen.generate().copyout(long_prefix, sizeof long_prefix);
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
