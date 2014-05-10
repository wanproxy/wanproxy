/*
 * Copyright (c) 2010-2013 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
