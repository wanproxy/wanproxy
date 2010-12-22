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
			wc->compressor_level_ = 9;
			break;
		case WANProxyConfigCompressorNone:
			wc->compressor_ = false;
			wc->compressor_level_ = 0;
			break;
		default:
			delete wc;

			ERROR("/wanproxy/config/codec") << "Invalid compressor type.";
			return (false);
		}
	} else {
		wc->compressor_ = false;
	}

	ConfigTypeInt *compressor_levelct;
	ConfigValue *compressor_levelcv = co->get("compressor_level", &compressor_levelct);
	if (compressor_levelcv != NULL) {
		intmax_t compressor_level;
		if (!compressor_levelct->get(compressor_levelcv, &compressor_level)) {
			delete wc;

			return (false);
		}

		if (!wc->compressor_) {
			delete wc;

			ERROR("/wanproxy/config/codec") << "Compressor level set but no compressor.";
			return (false);
		}

		if (compressor_level < 0 || compressor_level > 9) {
			delete wc;

			ERROR("/wanproxy/config/codec") << "Compressor level must be in range 0..9 (inclusive.)";
			return (false);
		}
		wc->compressor_level_ = compressor_level;
	}

	object_codec_map_[co] = wc;

	return (true);
}
