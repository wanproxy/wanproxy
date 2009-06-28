#ifndef	WANPROXY_CONFIG_CLASS_CODEC_H
#define	WANPROXY_CONFIG_CLASS_CODEC_H

#include "wanproxy_config_type_codec.h"

class XCodec;

class WANProxyConfigClassCodec : public ConfigClass {
	std::map<ConfigObject *, XCodec *> object_codec_map_;
public:
	WANProxyConfigClassCodec(void)
	: ConfigClass("codec"),
	  object_codec_map_()
	{
		add_member("codec", &wanproxy_config_type_codec);
	}

	~WANProxyConfigClassCodec()
	{ }

	bool activate(ConfigObject *);

	XCodec *get(ConfigObject *co) const
	{
		std::map<ConfigObject *, XCodec *>::const_iterator it;
		it = object_codec_map_.find(co);
		if (it == object_codec_map_.end())
			return (NULL);
		return (it->second);
	}
};

extern WANProxyConfigClassCodec wanproxy_config_class_codec;

#endif /* !WANPROXY_CONFIG_CLASS_CODEC_H */
