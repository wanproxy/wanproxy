#ifndef	WANPROXY_CONFIG_CLASS_CODEC_H
#define	WANPROXY_CONFIG_CLASS_CODEC_H

#include "wanproxy_config_type_codec.h"

class WANProxyConfigClassCodec : public ConfigClass {
public:
	WANProxyConfigClassCodec(void)
	: ConfigClass("codec")
	{
		add_member("codec", &wanproxy_config_type_codec);
	}

	~WANProxyConfigClassCodec()
	{ }

	bool activate(ConfigObject *);
};

extern WANProxyConfigClassCodec wanproxy_config_class_codec;

#endif /* !WANPROXY_CONFIG_CLASS_CODEC_H */
