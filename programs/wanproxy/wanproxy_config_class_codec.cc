#include <common/buffer.h>

#include <config/config_class.h>
#include <config/config_object.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>

#include "wanproxy_config_class_codec.h"

WANProxyConfigClassCodec wanproxy_config_class_codec;

bool
WANProxyConfigClassCodec::Instance::activate(const ConfigObject *co)
{
	codec_.name_ = co->name_;

	switch (codec_type_) {
	case WANProxyConfigCodecXCodec: {
		/*
		 * XXX
		 * Fetch UUID from permanent storage if there is any.
		 */
		UUID uuid;
		uuid.generate();

		XCodecCache *cache = XCodecCache::lookup(uuid);
		if (cache == NULL) {
			cache = new XCodecMemoryCache(uuid);
			XCodecCache::enter(uuid, cache);
		}
		XCodec *xcodec = new XCodec(cache);

		codec_.codec_ = xcodec;
		break;
	}
	case WANProxyConfigCodecNone:
		codec_.codec_ = NULL;
		break;
	default:
		ERROR("/wanproxy/config/codec") << "Invalid codec type.";
		return (false);
	}

	switch (compressor_) {
	case WANProxyConfigCompressorZlib:
		if (compressor_level_ < 0 || compressor_level_ > 9) {
			ERROR("/wanproxy/config/codec") << "Compressor level must be in range 0..9 (inclusive.)";
			return (false);
		}

		codec_.compressor_ = true;
		codec_.compressor_level_ = compressor_level_;
		break;
	case WANProxyConfigCompressorNone:
		if (compressor_level_ != -1) {
			ERROR("/wanproxy/config/codec") << "Compressor level set but no compressor.";
			return (false);
		}

		codec_.compressor_ = false;
		codec_.compressor_level_ = 0;
		break;
	default:
		ERROR("/wanproxy/config/codec") << "Invalid compressor type.";
		return (false);
	}

	return (true);
}
