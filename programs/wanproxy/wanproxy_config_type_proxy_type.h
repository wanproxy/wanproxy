#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_TYPE_PROXY_TYPE_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_TYPE_PROXY_TYPE_H

#include <config/config_type_enum.h>

enum WANProxyConfigProxyType {
	WANProxyConfigProxyTypeTCPTCP,
	WANProxyConfigProxyTypeUDPTCP,
	WANProxyConfigProxyTypeTCPUDP,
	WANProxyConfigProxyTypeUDPUDP,
};

typedef ConfigTypeEnum<WANProxyConfigProxyType> WANProxyConfigTypeProxyType;

extern WANProxyConfigTypeProxyType wanproxy_config_type_proxy_type;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_TYPE_PROXY_TYPE_H */
