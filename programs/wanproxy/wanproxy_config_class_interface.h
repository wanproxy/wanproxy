#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_INTERFACE_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_INTERFACE_H

#include <config/config_class_address.h>

class WANProxyConfigClassInterface : public ConfigClassAddress {
public:
	WANProxyConfigClassInterface(void)
	: ConfigClassAddress("interface")
	{ }

	~WANProxyConfigClassInterface()
	{ }
};

extern WANProxyConfigClassInterface wanproxy_config_class_interface;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_INTERFACE_H */
