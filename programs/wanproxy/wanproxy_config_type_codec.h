#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_TYPE_CODEC_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_TYPE_CODEC_H

#include <config/config_type_enum.h>

enum WANProxyConfigCodec {
	WANProxyConfigCodecNone,
	WANProxyConfigCodecXCodec
};

typedef ConfigTypeEnum<WANProxyConfigCodec> WANProxyConfigTypeCodec;

extern WANProxyConfigTypeCodec wanproxy_config_type_codec;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_TYPE_CODEC_H */
