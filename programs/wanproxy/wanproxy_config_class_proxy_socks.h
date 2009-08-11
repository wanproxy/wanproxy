#ifndef	WANPROXY_CONFIG_CLASS_PROXY_SOCKS_H
#define	WANPROXY_CONFIG_CLASS_PROXY_SOCKS_H

#include <config/config_type_pointer.h>

class ProxySocksListener;

class WANProxyConfigClassProxySocks : public ConfigClass {
	std::map<ConfigObject *, ProxySocksListener *> object_listener_map_;
public:
	WANProxyConfigClassProxySocks(void)
	: ConfigClass("proxy-socks"),
	  object_listener_map_()
	{
		add_member("interface", &config_type_pointer);
	}

	/* XXX So wrong.  */
	~WANProxyConfigClassProxySocks()
	{ }

	bool activate(ConfigObject *);
};

extern WANProxyConfigClassProxySocks wanproxy_config_class_proxy_socks;

#endif /* !WANPROXY_CONFIG_CLASS_PROXY_SOCKS_H */
