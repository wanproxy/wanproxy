/*
 * Copyright (c) 2011 Juli Mallett. All rights reserved.
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

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>
#include <event/speed_test.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_hash.h>

static uint8_t zbuf[XCODEC_SEGMENT_LENGTH * 32];

class XCodecHashSpeed : SpeedTest {
	uintmax_t bytes_;
	uint64_t hash_;
public:
	XCodecHashSpeed(void)
	: bytes_(0),
	  hash_(0)
	{
		perform();
	}

	~XCodecHashSpeed()
	{ }

private:
	void perform(void)
	{
		unsigned i;

		for (i = 0; i < sizeof zbuf; i += XCODEC_SEGMENT_LENGTH)
			hash_ += XCodecHash::hash(zbuf + i);
		zbuf[i] = hash_; /* So the compiler [hopefully] won't optimize out any iterations.  */

		bytes_ += sizeof zbuf;

		schedule();
	}

	void finish(void)
	{
		INFO("/example/xcodec/hash/speed1") << "Timer expired; " << bytes_ << " bytes hashed (final hash " << hash_ << ").";
	}
};

int
main(void)
{
	unsigned i;
	for (i = 0; i < sizeof zbuf; i++)
		zbuf[i] = random();

	XCodecHashSpeed *cs = new XCodecHashSpeed();

	event_main();

	delete cs;
}
