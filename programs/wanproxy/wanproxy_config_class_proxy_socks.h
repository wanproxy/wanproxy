#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PROXY_SOCKS_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PROXY_SOCKS_H

#include <config/config_type_pointer.h>

class ProxySocksListener;

class WANProxyConfigClassProxySocks : public ConfigClass {
	struct Instance : public ConfigClassInstance {
		ConfigObject *interface_;

		Instance(void)
		: interface_(NULL)
		{ }

		bool activate(const ConfigObject *);
	};
public:
	WANProxyConfigClassProxySocks(void)
	: ConfigClass("proxy-socks", new ConstructorFactory<ConfigClassInstance, Instance>)
	{
		add_member("interface", &config_type_pointer, &Instance::interface_);
	}

	/* XXX So wrong.  */
	~WANProxyConfigClassProxySocks()
	{ }
};

extern WANProxyConfigClassProxySocks wanproxy_config_class_proxy_socks;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PROXY_SOCKS_H */
