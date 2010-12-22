#ifndef	WANPROXY_CONFIG_CLASS_CODEC_H
#define	WANPROXY_CONFIG_CLASS_CODEC_H

#include <config/config_type_int.h>

#include "wanproxy_codec.h"
#include "wanproxy_config_type_codec.h"
#include "wanproxy_config_type_compressor.h"

class WANProxyConfigClassCodec : public ConfigClass {
	std::map<ConfigObject *, WANProxyCodec *> object_codec_map_;
public:
	WANProxyConfigClassCodec(void)
	: ConfigClass("codec"),
	  object_codec_map_()
	{
		add_member("codec", &wanproxy_config_type_codec);
		add_member("compressor", &wanproxy_config_type_compressor);
		add_member("compressor_level", &config_type_int);
	}

	~WANProxyConfigClassCodec()
	{ }

	bool activate(ConfigObject *);

	WANProxyCodec *get(ConfigObject *co) const
	{
		std::map<ConfigObject *, WANProxyCodec *>::const_iterator it;
		it = object_codec_map_.find(co);
		if (it == object_codec_map_.end())
			return (NULL);
		return (it->second);
	}
};

extern WANProxyConfigClassCodec wanproxy_config_class_codec;

#endif /* !WANPROXY_CONFIG_CLASS_CODEC_H */
