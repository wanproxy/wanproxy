#include <common/buffer.h>

#include <config/config_class.h>
#include <config/config_object.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/socket_types.h>

#include "proxy_socks_listener.h"
#include "wanproxy_config_class_interface.h"
#include "wanproxy_config_class_proxy_socks.h"

WANProxyConfigClassProxySocks wanproxy_config_class_proxy_socks;

bool
WANProxyConfigClassProxySocks::Instance::activate(const ConfigObject *co)
{
	WANProxyConfigClassInterface::Instance *interface =
		dynamic_cast<WANProxyConfigClassInterface::Instance *>(interface_->instance_);
	if (interface == NULL)
		return (false);

	if (interface->host_ == "" || interface->port_ == "")
		return (false);
	std::string interface_address = '[' + interface->host_ + ']' + ':' + interface->port_;

	new ProxySocksListener(co->name_, interface->family_, interface_address);

	return (true);
}
