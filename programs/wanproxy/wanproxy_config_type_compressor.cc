#include "wanproxy_config_type_compressor.h"

static struct WANProxyConfigTypeCompressor::Mapping wanproxy_config_type_compressor_map[] = {
	{ "zlib",	WANProxyConfigCompressorZlib },
	{ "None",	WANProxyConfigCompressorNone },
	{ NULL,		WANProxyConfigCompressorNone }
};

WANProxyConfigTypeCompressor
	wanproxy_config_type_compressor("compressor", wanproxy_config_type_compressor_map);
