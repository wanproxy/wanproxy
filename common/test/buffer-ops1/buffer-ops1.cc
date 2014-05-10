/*
 * Copyright (c) 2008-2013 Juli Mallett. All rights reserved.
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

static unsigned data_sizes[] = {
	1, 2, 3, 4, 7, 8, 11, 13,
	BUFFER_SEGMENT_SIZE /99, BUFFER_SEGMENT_SIZE /40,
	BUFFER_SEGMENT_SIZE /32, BUFFER_SEGMENT_SIZE /25,
	BUFFER_SEGMENT_SIZE /19, BUFFER_SEGMENT_SIZE /17,
	BUFFER_SEGMENT_SIZE /15, BUFFER_SEGMENT_SIZE /13,
	BUFFER_SEGMENT_SIZE /11, BUFFER_SEGMENT_SIZE /10,
	BUFFER_SEGMENT_SIZE / 9, BUFFER_SEGMENT_SIZE / 8,
	BUFFER_SEGMENT_SIZE / 7, BUFFER_SEGMENT_SIZE / 6,
	BUFFER_SEGMENT_SIZE / 5, BUFFER_SEGMENT_SIZE / 4,
	BUFFER_SEGMENT_SIZE / 3, BUFFER_SEGMENT_SIZE / 2,
	BUFFER_SEGMENT_SIZE,
	BUFFER_SEGMENT_SIZE * 2, BUFFER_SEGMENT_SIZE * 3,
	BUFFER_SEGMENT_SIZE * 4, BUFFER_SEGMENT_SIZE * 5,
	BUFFER_SEGMENT_SIZE * 6, BUFFER_SEGMENT_SIZE * 7,
	BUFFER_SEGMENT_SIZE * 8, BUFFER_SEGMENT_SIZE * 9,
	BUFFER_SEGMENT_SIZE *10, BUFFER_SEGMENT_SIZE *11,
	BUFFER_SEGMENT_SIZE *13, BUFFER_SEGMENT_SIZE *15,
	BUFFER_SEGMENT_SIZE *17, BUFFER_SEGMENT_SIZE *19,
	BUFFER_SEGMENT_SIZE *25, BUFFER_SEGMENT_SIZE *32,
	BUFFER_SEGMENT_SIZE *40, BUFFER_SEGMENT_SIZE *99
};
#define	DATA_SIZES	(sizeof data_sizes / sizeof data_sizes[0])

static uint8_t *data_buffers[DATA_SIZES];

static void run_tests(TestGroup&, unsigned, unsigned, unsigned);

int
main(void)
{
	unsigned i, j, k;
	unsigned o;

	srandom(time(NULL));

	for (i = 0; i < DATA_SIZES; i++) {
		data_buffers[i] = new uint8_t[data_sizes[i]];
		for (o = 0; o < data_sizes[i]; o++)
			data_buffers[i][o] = random();
	}

	TestGroup g("/test/buffer/ops1", "Buffer (operations) #1");
	for (i = 0; i < DATA_SIZES; i++)
		for (j = 0; j < DATA_SIZES; j++)
			for (k = 0; k < DATA_SIZES; k++)
				run_tests(g, i, j, k);
	return (0);
}

static void
run_tests(TestGroup& g, unsigned i, unsigned j, unsigned k)
{
	Test _(g, "run_tests(i, j, k)");
	const uint8_t *ip, *jp, *kp;
	size_t in, jn, kn;

	ip = data_buffers[i];
	in = data_sizes[i];
	uint8_t ib[in];

	jp = data_buffers[j];
	jn = data_sizes[j];
	uint8_t jb[jn];

	kp = data_buffers[k];
	kn = data_sizes[k];
	uint8_t kb[kn];

	Buffer bi(ip, in), bj(jp, jn), bk(kp, kn);
	Buffer buf;
	buf.append(std::string("["));
	buf.append(bi);
	buf.append((const uint8_t *)",", 1);
	buf.append(&bj);
	buf.append(std::string(",", 1));
	buf.append(&bk, bk.length());
	buf.append(Buffer(Buffer("]")));

	if (buf.empty())
		return;
	if (buf.length() != 1 + in + 1 + jn + 1 + kn + 1)
		return;
	if (buf.peek() != '[')
		return;
	buf.copyout(ib, 1, in);
	if (memcmp(ib, ip, in) != 0)
		return;
	buf.moveout(ib, 1, in);
	if (memcmp(ib, ip, in) != 0)
		return;
	if (buf.length() != 1 + jn + 1 + kn + 1)
		return;
	if (buf.peek() != ',')
		return;
	buf.skip(1);
	buf.moveout(jb, 0, jn);
	if (memcmp(jb, jp, jn) != 0)
		return;
	if (buf.length() != 1 + kn + 1)
		return;
	buf.skip(1);
	buf.copyout(kb, kn);
	buf.skip(kn);
	if (buf.length() != 1)
		return;
	unsigned o;
	o = ~0;
	if (!buf.find(']', &o))
		return;
	if (o != 0)
		return;
	buf = std::string("");
	if (!buf.empty())
		return;
	buf = bj;
	if (jn != 0) {
		if (buf.empty())
			return;
		if (buf.empty())
			return;
		if (buf.length() != jn)
			return;
		if (jn == 1) {
			buf.skip(jn);
		} else {
			buf.moveout(jb, 1, jn - 1);
			if (memcmp(jb, jp + 1, jn - 1) != 0)
				return;
		}
	}
	if (buf.length() != 0)
		return;
	if (!buf.empty())
		return;
	if (!buf.empty())
		return;

	if (bi.length() != in)
		return;
	if (bj.length() != jn)
		return;
	if (bk.length() != kn)
		return;

	bi.clear();
	if (bi.length() != 0)
		return;
	if (!bi.empty())
		return;
	bj.clear();
	if (bj.length() != 0)
		return;
	if (!bj.empty())
		return;
	bk.clear();
	if (bk.length() != 0)
		return;
	_.pass();
}
