#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PROXY_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PROXY_H

#include <config/config_type_pointer.h>

#include "wanproxy_config_type_proxy_type.h"

class ProxyListener;

class WANProxyConfigClassProxy : public ConfigClass {
	struct Instance : public ConfigClassInstance {
		WANProxyConfigProxyType type_;
		ConfigObject *interface_;
		ConfigObject *interface_codec_;
		ConfigObject *peer_;
		ConfigObject *peer_codec_;

		Instance(void)
		: type_(WANProxyConfigProxyTypeTCPTCP),
		  interface_(NULL),
		  interface_codec_(NULL),
		  peer_(NULL),
		  peer_codec_(NULL)
		{ }

		bool activate(const ConfigObject *);
	};
public:
	WANProxyConfigClassProxy(void)
	: ConfigClass("proxy", new ConstructorFactory<ConfigClassInstance, Instance>)
	{
		add_member("type", &wanproxy_config_type_proxy_type, &Instance::type_);
		add_member("interface", &config_type_pointer, &Instance::interface_);
		add_member("interface_codec", &config_type_pointer, &Instance::interface_codec_);
		add_member("peer", &config_type_pointer, &Instance::peer_);
		add_member("peer_codec", &config_type_pointer, &Instance::peer_codec_);
	}

	/* XXX So wrong.  */
	~WANProxyConfigClassProxy()
	{ }
};

extern WANProxyConfigClassProxy wanproxy_config_class_proxy;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PROXY_H */
