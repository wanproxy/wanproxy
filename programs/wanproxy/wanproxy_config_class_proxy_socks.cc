#include <common/buffer.h>

#include <config/config_class.h>
#include <config/config_object.h>
#include <config/config_value.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/socket_types.h>

#include "proxy_socks_listener.h"
#include "wanproxy_config_class_interface.h"
#include "wanproxy_config_class_proxy_socks.h"

WANProxyConfigClassProxySocks wanproxy_config_class_proxy_socks;

bool
WANProxyConfigClassProxySocks::activate(ConfigObject *co)
{
	if (object_listener_map_.find(co) != object_listener_map_.end())
		return (false);

	/* Extract interface.  */
	ConfigTypePointer *interfacect;
	ConfigValue *interfacecv = co->get("interface", &interfacect);
	if (interfacecv == NULL)
		return (false);

	WANProxyConfigClassInterface *interfacecc;
	ConfigObject *interfaceco;
	if (!interfacect->get(interfacecv, &interfaceco, &interfacecc))
		return (false);

	ConfigTypeAddressFamily *interface_familyct;
	ConfigValue *interface_familycv = interfaceco->get("family", &interface_familyct);
	if (interface_familycv == NULL)
		return (false);

	SocketAddressFamily interface_family;
	if (!interface_familyct->get(interface_familycv, &interface_family))
		return (false);

	ConfigTypeString *interface_hostct;
	ConfigValue *interface_hostcv = interfaceco->get("host", &interface_hostct);
	if (interface_hostcv == NULL)
		return (false);

	std::string interface_hoststr;
	if (!interface_hostct->get(interface_hostcv, &interface_hoststr))
		return (false);

	ConfigTypeString *interface_portct;
	ConfigValue *interface_portcv = interfaceco->get("port", &interface_portct);
	if (interface_portcv == NULL)
		return (false);

	std::string interface_portstr;
	if (!interface_portct->get(interface_portcv, &interface_portstr))
		return (false);

	std::string interface_address = '[' + interface_hoststr + ']' + ':' + interface_portstr;
	ProxySocksListener *listener = new ProxySocksListener(co->name(), interface_family, interface_address);
	object_listener_map_[co] = listener;

	return (true);
}
