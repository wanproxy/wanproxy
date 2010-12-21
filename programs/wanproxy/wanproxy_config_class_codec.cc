#include <common/buffer.h>

#include <config/config_class.h>
#include <config/config_object.h>
#include <config/config_value.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>

#include "wanproxy_config_class_codec.h"

WANProxyConfigClassCodec wanproxy_config_class_codec;

bool
WANProxyConfigClassCodec::activate(ConfigObject *co)
{
	if (object_codec_map_.find(co) != object_codec_map_.end())
		return (false);

	WANProxyCodec *wc = new WANProxyCodec(co->name());

	WANProxyConfigTypeCodec *codecct;
	ConfigValue *codeccv = co->get("codec", &codecct);
	if (codeccv != NULL) {
		WANProxyConfigCodec codec;
		if (!codecct->get(codeccv, &codec)) {
			delete wc;

			return (false);
		}

		switch (codec) {
		case WANProxyConfigCodecXCodec: {
			/*
			 * XXX
			 * Fetch UUID from permanent storage if there is any.
			 */
			UUID uuid;
			uuid.generate();

			XCodecCache *cache = XCodecCache::lookup(uuid);
			XCodec *xcodec = new XCodec(cache);

			wc->codec_ = xcodec;
			break;
		}
		case WANProxyConfigCodecNone:
			wc->codec_ = NULL;
			break;
		default:
			delete wc;

			ERROR("/wanproxy/config/codec") << "Invalid codec type.";
			return (false);
		}
	} else {
		wc->codec_ = NULL;
	}

	WANProxyConfigTypeCompressor *compressorct;
	ConfigValue *compressorcv = co->get("compressor", &compressorct);
	if (compressorcv != NULL) {
		WANProxyConfigCompressor compressor;
		if (!compressorct->get(compressorcv, &compressor)) {
			delete wc;

			return (false);
		}

		switch (compressor) {
		case WANProxyConfigCompressorZlib:
			wc->compressor_ = true;
			break;
		case WANProxyConfigCompressorNone:
			wc->compressor_ = false;
			break;
		default:
			delete wc;

			ERROR("/wanproxy/config/codec") << "Invalid compressor type.";
			return (false);
		}
	} else {
		wc->compressor_ = false;
	}

	object_codec_map_[co] = wc;

	return (true);
}
