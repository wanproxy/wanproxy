#include <string>

#include "wanproxy_config_type_codec.h"

struct WANProxyConfigTypeCodec::Mapping wanproxy_config_type_codec_map[] = {
	{ "XCodec",	WANProxyConfigCodecXCodec },
	{ "None",	WANProxyConfigCodecNone },
	{ NULL,		WANProxyConfigCodecNone }
};

WANProxyConfigTypeCodec
	wanproxy_config_type_codec("codec", wanproxy_config_type_codec_map);
