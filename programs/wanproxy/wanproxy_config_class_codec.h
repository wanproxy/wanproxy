#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_CODEC_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_CODEC_H

#include <config/config_type_int.h>

#include "wanproxy_codec.h"
#include "wanproxy_config_type_codec.h"
#include "wanproxy_config_type_compressor.h"

class WANProxyConfigClassCodec : public ConfigClass {
public:
	struct Instance : public ConfigClassInstance {
		WANProxyCodec codec_;
		WANProxyConfigCodec codec_type_;
		WANProxyConfigCompressor compressor_;
		intmax_t compressor_level_;

		Instance(void)
		: codec_(),
		  codec_type_(WANProxyConfigCodecNone),
		  compressor_(WANProxyConfigCompressorNone),
		  compressor_level_(0)
		{ }

		bool activate(const ConfigObject *);
	};

	WANProxyConfigClassCodec(void)
	: ConfigClass("codec", new ConstructorFactory<ConfigClassInstance, Instance>)
	{
		add_member("codec", &wanproxy_config_type_codec, &Instance::codec_type_);
		add_member("compressor", &wanproxy_config_type_compressor, &Instance::compressor_);
		add_member("compressor_level", &config_type_int, &Instance::compressor_level_);
	}

	~WANProxyConfigClassCodec()
	{ }
};

extern WANProxyConfigClassCodec wanproxy_config_class_codec;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_CODEC_H */
