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

	WANProxyConfigTypeCodec *codecct;
	ConfigValue *codeccv = co->get("codec", &codecct);
	if (codeccv == NULL)
		return (true);

	WANProxyConfigCodec codec;
	if (!codecct->get(codeccv, &codec))
		return (false);

	switch (codec) {
	case WANProxyConfigCodecXCodec: {
		XCodecCache *cache = new XCodecCache(); /* XXX other cache methods?  */
		XCodec *xcodec = new XCodec(cache);
		object_codec_map_[co] = xcodec;
		break;
	}
	case WANProxyConfigCodecNone:
		/* Explicitly use no codec.  */
		break;
	default:
		ERROR("/wanproxy/config/codec") << "Invalid codec type.";
		return (false);
	}

	return (true);
}
