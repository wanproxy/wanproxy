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

		uint64_t sum = 0xfeedfacecafebabeull * n;
		uint8_t fill[n * FILL_LENGTH];
		unsigned i;
		for (i = 0; i < sizeof fill; i++) {
			sum += fill[i] = random() % FILL_BYTE;
		}

		for (i = 0; i < FILL_NUMBER; i++) {
			big.append(fill, sizeof fill);
		}
		sum *= FILL_NUMBER;
		{
			Test _(g, "Expected length.");
			if (big.length() == n * FILL_LENGTH * FILL_NUMBER + headerlen)
				_.pass();
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

			Buffer::SegmentIterator iter = big.segments();
			while (!iter.end()) {
				const BufferSegment *seg = *iter;
				const uint8_t *p;
				const uint8_t *q = seg->end();
				for (p = seg->data(); p < q; p++)
					sum -= *p;
				iter.next();
			}
			big.clear();

			if (sum == 0xfeedfacecafebabeull * n * FILL_NUMBER)
				_.pass();
		}
	}

	return (0);
}
