#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PEER_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PEER_H

#include <config/config_class_address.h>

class WANProxyConfigClassPeer : public ConfigClassAddress {
public:
	WANProxyConfigClassPeer(void)
	: ConfigClassAddress("peer")
	{ }

	~WANProxyConfigClassPeer()
	{ }
};

extern WANProxyConfigClassPeer wanproxy_config_class_peer;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PEER_H */
