#include "wanproxy_config_type_proxy_type.h"

static struct WANProxyConfigTypeProxyType::Mapping wanproxy_config_type_proxy_type_map[] = {
	{ "TCP",	WANProxyConfigProxyTypeTCPTCP },
	{ "TCP-TCP",	WANProxyConfigProxyTypeTCPTCP },
	{ "SSH",	WANProxyConfigProxyTypeSSHSSH },
	{ "SSH-SSH",	WANProxyConfigProxyTypeSSHSSH },
	{ NULL,		WANProxyConfigProxyTypeTCPTCP }
};

WANProxyConfigTypeProxyType
	wanproxy_config_type_proxy_type("proxy_type", wanproxy_config_type_proxy_type_map);
