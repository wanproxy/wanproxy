/*
 * Copyright (c) 2011-2013 Juli Mallett. All rights reserved.
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
#include <common/uuid/uuid.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>
#include <xcodec/xcodec_decoder.h>
#include <xcodec/xcodec_encoder.h>

int
main(void)
{
	{
		TestGroup g("/test/xcodec/encode-decode/1/char_kat", "XCodecEncoder::encode / XCodecDecoder::decode #1");

		unsigned i;
		for (i = 0; i < 256; i++) {
			Buffer in;
			unsigned j;
			for (j = 0; j < XCODEC_SEGMENT_LENGTH; j++)
				in.append((uint8_t)i);

			for (j = 0; j < 8; j++) {
				Buffer tmp(in);
				tmp.append(in);
				in = tmp;
			}

			Buffer original(in);

			UUID uuid;
			uuid.generate();

			XCodecCache *cache = new XCodecMemoryCache(uuid);
			XCodecEncoder encoder(cache);

			Buffer out;
			encoder.encode(&out, &in);

			{
				Test _(g, "Empty input buffer after encode.", in.empty());
			}

			{
				Test _(g, "Non-empty output buffer after encode.", !out.empty());
			}

			{
				Test _(g, "Reduction in size.", out.length() < original.length());
			}

			out.moveout(&in);

			XCodecDecoder decoder(cache);
			std::set<uint64_t> unknown_hashes;

			bool ok = decoder.decode(&out, &in, unknown_hashes);
			{
				Test _(g, "Decoder success.", ok);
			}

			{
				Test _(g, "No unknown hashes.", unknown_hashes.empty());
			}

			{
				Test _(g, "Empty input buffer after decode.", in.empty());
			}

			{
				Test _(g, "Non-empty output buffer after decode.", !out.empty());
			}

			{
				Test _(g, "Expected data.", out.equal(&original));
			}

			delete cache;
		}
	}

	return (0);
}
