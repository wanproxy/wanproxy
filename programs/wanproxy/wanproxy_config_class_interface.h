#ifndef	WANPROXY_CONFIG_CLASS_INTERFACE_H
#define	WANPROXY_CONFIG_CLASS_INTERFACE_H

#include <config/config_class_address.h>

class WANProxyConfigClassInterface : public ConfigClassAddress {
public:
	WANProxyConfigClassInterface(void)
	: ConfigClassAddress("interface")
	{ }

	~WANProxyConfigClassInterface()
	{ }

	bool activate(ConfigObject *);
};

extern WANProxyConfigClassInterface wanproxy_config_class_interface;

#endif /* !WANPROXY_CONFIG_CLASS_INTERFACE_H */
