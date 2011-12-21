#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_TYPE_COMPRESSOR_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_TYPE_COMPRESSOR_H

#include <config/config_type_enum.h>

enum WANProxyConfigCompressor {
	WANProxyConfigCompressorNone,
	WANProxyConfigCompressorZlib
};

typedef ConfigTypeEnum<WANProxyConfigCompressor> WANProxyConfigTypeCompressor;

extern WANProxyConfigTypeCompressor wanproxy_config_type_compressor;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_TYPE_COMPRESSOR_H */
