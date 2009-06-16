#ifndef	WANPROXY_CONFIG_CLASS_PROXY_H
#define	WANPROXY_CONFIG_CLASS_PROXY_H

#include <config/config_type_pointer.h>

class WANProxyConfigClassProxy : public ConfigClass {
public:
	WANProxyConfigClassProxy(void)
	: ConfigClass("proxy")
	{
		add_member("interface", &config_type_pointer);
		add_member("decoder", &config_type_pointer);
		add_member("peer", &config_type_pointer);
		add_member("encoder", &config_type_pointer);
	}

	/* XXX So wrong.  */
	~WANProxyConfigClassProxy()
	{ }

	bool activate(ConfigObject *);
};

extern WANProxyConfigClassProxy wanproxy_config_class_proxy;

#endif /* !WANPROXY_CONFIG_CLASS_PROXY_H */
