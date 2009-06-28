#ifndef	WANPROXY_SOCKS_CONFIG_CLASS_PROXY_SOCKS_H
#define	WANPROXY_SOCKS_CONFIG_CLASS_PROXY_SOCKS_H

#include <config/config_type_pointer.h>

class ProxySocksListener;

class WANProxySocksConfigClassProxySocks : public ConfigClass {
	std::map<ConfigObject *, ProxySocksListener *> object_listener_map_;
public:
	WANProxySocksConfigClassProxySocks(void)
	: ConfigClass("proxy-socks"),
	  object_listener_map_()
	{
		add_member("interface", &config_type_pointer);
	}

	/* XXX So wrong.  */
	~WANProxySocksConfigClassProxySocks()
	{ }

	bool activate(ConfigObject *);
};

extern WANProxySocksConfigClassProxySocks wanproxy_config_class_proxy_socks;

#endif /* !WANPROXY_SOCKS_CONFIG_CLASS_PROXY_SOCKS_H */
