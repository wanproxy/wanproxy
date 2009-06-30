#include <common/buffer.h>

#include <config/config_class.h>
#include <config/config_object.h>
#include <config/config_value.h>

#include <event/action.h>
#include <event/event.h>

#include "proxy_socks_listener.h"
#include "wanproxy_config_class_interface.h"
#include "wanproxy_config_class_proxy_socks.h"

WANProxySocksConfigClassProxySocks wanproxy_config_class_proxy_socks;

bool
WANProxySocksConfigClassProxySocks::activate(ConfigObject *co)
{
	if (object_listener_map_.find(co) != object_listener_map_.end())
		return (false);

	/* Extract interface.  */
	ConfigValue *interfacecv = co->members_["interface"];
	if (interfacecv == NULL)
		return (false);

	ConfigTypePointer *interfacect = dynamic_cast<ConfigTypePointer *>(interfacecv->type_);
	if (interfacect == NULL)
		return (false);

	ConfigObject *interfaceco;
	if (!interfacect->get(interfacecv, &interfaceco))
		return (false);

	WANProxyConfigClassInterface *interfacecc = dynamic_cast<WANProxyConfigClassInterface *>(interfaceco->class_);
	if (interfacecc == NULL)
		return (false);

	ConfigValue *interface_hostcv = interfaceco->members_["host"];
	if (interface_hostcv == NULL)
		return (false);

	ConfigTypeString *interface_hostct = dynamic_cast<ConfigTypeString *>(interface_hostcv->type_);
	if (interface_hostct == NULL)
		return (false);

	std::string interface_hoststr;
	if (!interface_hostct->get(interface_hostcv, &interface_hoststr))
		return (false);

	ConfigValue *interface_portcv = interfaceco->members_["port"];
	if (interface_portcv == NULL)
		return (false);

	ConfigTypeInt *interface_portct = dynamic_cast<ConfigTypeInt *>(interface_portcv->type_);
	if (interface_portct == NULL)
		return (false);

	intmax_t interface_portint;
	if (!interface_portct->get(interface_portcv, &interface_portint))
		return (false);

	ProxySocksListener *listener = new ProxySocksListener(interface_hoststr, interface_portint);
	object_listener_map_[co] = listener;

	return (true);
}
