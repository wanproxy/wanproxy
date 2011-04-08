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

			XCodecCache *cache = XCodecCache::lookup(uuid);
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

			in = out;
			out.clear();

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
		}
	}

	return (0);
}
