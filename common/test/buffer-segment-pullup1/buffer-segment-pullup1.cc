/*
 * Copyright (c) 2008-2011 Juli Mallett. All rights reserved.
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

int
main(void)
{
	{
		TestGroup g("/test/buffer/pullup1", "BufferSegment::pullup #1");

		BufferSegment *seg = BufferSegment::create();
		seg->append("ABCD");
		{
			Test _(g, "BufferSegment equal");
			if (seg->equal("ABCD") && seg->length() == 4)
				_.pass();
		}
		{
			Test _(g, "BufferSegment skip");
			BufferSegment *seg2 = seg->skip(1);
			if (seg2 == seg && seg->equal("BCD") &&
			    seg->length() == 3)
				_.pass();
		}
		{
			Test _(g, "BufferSegment append");
			BufferSegment *seg2 = seg->append('E');
			if (seg2 == seg && seg->equal("BCDE") &&
			    seg->length() == 4)
				_.pass();
		}
		{
			Test _(g, "BufferSegment COW skip");
			seg->ref();
			BufferSegment *seg2 = seg->skip(1);
			if (seg2 != seg && seg2->equal("CDE") &&
			    seg2->length() == 3 && !seg2->equal(seg) &&
			    seg->equal("BCDE") && seg->length() == 4)
				_.pass();
			seg2->unref();
		}
		{
			Test _(g, "BufferSegment COW append");
			seg->ref();
			BufferSegment *seg2 = seg->append('F');
			if (seg2 != seg && seg2->equal("BCDEF") &&
			    seg2->length() == 5 && !seg2->equal(seg) &&
			    seg->equal("BCDE") && seg->length() == 4)
				_.pass();
			seg2->unref();
		}
		{
			Test _(g, "BufferSegment post-COW skip");
			BufferSegment *seg2 = seg->skip(2);
			if (seg2 == seg && seg->equal("DE") &&
			    seg2->length() == 2)
				_.pass();
		}
		{
			Test _(g, "BufferSegment post-COW append");
			BufferSegment *seg2 = seg->append('F');
			if (seg2 == seg && seg->equal("DEF") &&
			    seg2->length() == 3)
				_.pass();
		}
		{
			Test _(g, "BufferSegment append-to-full");
			while (seg->avail() > 0) {
				BufferSegment *seg2 = seg->append('X');
				ASSERT("/test/buffer/pullup1", seg == seg2);
			}
			_.pass();
		}
		{
			Test _(g, "BufferSegment skip leading + check");
			BufferSegment *seg2 = seg->skip(4);
			if (seg2 == seg) {
				const uint8_t *p;
				for (p = seg->data(); p < seg->end(); p++) {
					if (*p != 'X')
						break;
				}
				if (p == seg->end())
					_.pass();
			}
		}
		{
			Test _(g, "BufferSegment append-to-full 2");
			while (seg->avail() > 0) {
				BufferSegment *seg2 = seg->append('X');
				ASSERT("/test/buffer/pullup1", seg == seg2);
			}
			_.pass();
		}
		{
			Test _(g, "BufferSegment check full");
			const uint8_t *p;
			for (p = seg->data(); p < seg->end(); p++) {
				if (*p != 'X')
					break;
			}
			if (p == seg->end())
				_.pass();
		}
		seg->unref();
	}

	{
		TestGroup g("/test/buffer/pullup2", "BufferSegment::pullup #2");
		uint8_t data[BUFFER_SEGMENT_SIZE / 2];
		unsigned i;

		for (i = 0; i < sizeof data; i++)
			data[i] = random() & 0xff;

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
