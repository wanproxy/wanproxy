#ifndef	WANPROXY_CONFIG_TYPE_PROXY_TYPE_H
#define	WANPROXY_CONFIG_TYPE_PROXY_TYPE_H

#include <config/config_type_enum.h>

enum WANProxyConfigProxyType {
	WANProxyConfigProxyTypeTCPTCP,
	WANProxyConfigProxyTypeUDPTCP,
	WANProxyConfigProxyTypeTCPUDP,
	WANProxyConfigProxyTypeUDPUDP,
	WANProxyConfigProxyTypeSSHSSH,
};

typedef ConfigTypeEnum<WANProxyConfigProxyType> WANProxyConfigTypeProxyType;

extern WANProxyConfigTypeProxyType wanproxy_config_type_proxy_type;

#endif /* !WANPROXY_CONFIG_TYPE_PROXY_TYPE_H */
