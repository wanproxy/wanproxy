#include <common/buffer.h>

#include <config/config_class.h>
#include <config/config_object.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/socket_types.h>

#include "proxy_listener.h"
#include "ssh_proxy_listener.h"
#include "wanproxy_config_class_codec.h"
#include "wanproxy_config_class_interface.h"
#include "wanproxy_config_class_peer.h"
#include "wanproxy_config_class_proxy.h"

WANProxyConfigClassProxy wanproxy_config_class_proxy;

bool
WANProxyConfigClassProxy::Instance::activate(const ConfigObject *co)
{
	WANProxyConfigClassInterface::Instance *interface =
		dynamic_cast<WANProxyConfigClassInterface::Instance *>(interface_->instance_);
	if (interface == NULL)
		return (false);

	if (interface->host_ == "" || interface->port_ == "")
		return (false);

	WANProxyCodec *interface_codec;
	if (interface_codec_ != NULL) {
		WANProxyConfigClassCodec::Instance *codec =
			dynamic_cast<WANProxyConfigClassCodec::Instance *>(interface_codec_->instance_);
		if (codec == NULL)
			return (false);
		interface_codec = &codec->codec_;
	} else {
		interface_codec = NULL;
	}

	WANProxyConfigClassPeer::Instance *peer =
		dynamic_cast<WANProxyConfigClassPeer::Instance *>(peer_->instance_);
	if (peer == NULL)
		return (false);

	if (peer->host_ == "" || peer->port_ == "")
		return (false);

	WANProxyCodec *peer_codec;
	if (peer_codec_ != NULL) {
		WANProxyConfigClassCodec::Instance *codec =
			dynamic_cast<WANProxyConfigClassCodec::Instance *>(peer_codec_->instance_);
		if (codec == NULL)
			return (false);
		peer_codec = &codec->codec_;
	} else {
		peer_codec = NULL;
	}

	std::string interface_address = '[' + interface->host_ + ']' + ':' + interface->port_;
	std::string peer_address = '[' + peer->host_ + ']' + ':' + peer->port_;

	if (type_ == WANProxyConfigProxyTypeTCPTCP) {
		new ProxyListener(co->name_, interface_codec, peer_codec, interface->family_, interface_address, peer->family_, peer_address);
	} else {
		new SSHProxyListener(co->name_, interface_codec, peer_codec, interface->family_, interface_address, peer->family_, peer_address);
	}

	return (true);
}
