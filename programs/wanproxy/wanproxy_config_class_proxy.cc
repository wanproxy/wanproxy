#include <common/buffer.h>

#include <config/config_class.h>
#include <config/config_object.h>
#include <config/config_value.h>

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
WANProxyConfigClassProxy::activate(ConfigObject *co)
{
	if (object_listener_map_.find(co) != object_listener_map_.end())
		return (false);

	/* Extract type.  */
	WANProxyConfigTypeProxyType *typect;
	ConfigValue *typecv = co->get("type", &typect);
	if (typecv == NULL)
		return (false);

	WANProxyConfigProxyType type;
	if (!typect->get(typecv, &type))
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

	/* Extract interface_codec.  */
	ConfigTypePointer *interface_codecct;
	ConfigValue *interface_codeccv = co->get("interface_codec", &interface_codecct);
	if (interface_codeccv == NULL)
		return (false);

	WANProxyConfigClassCodec *interface_codeccc;
	ConfigObject *interface_codecco;
	if (!interface_codecct->get(interface_codeccv, &interface_codecco, &interface_codeccc))
		return (false);

	WANProxyCodec *interface_codeccodec;
	if (interface_codecco != NULL) {
		interface_codeccodec = interface_codeccc->get(interface_codecco);
	} else {
		interface_codeccodec = NULL;
	}

	/* Extract peer.  */
	ConfigTypePointer *peerct;
	ConfigValue *peercv = co->get("peer", &peerct);
	if (peercv == NULL)
		return (false);

	WANProxyConfigClassPeer *peercc;
	ConfigObject *peerco;
	if (!peerct->get(peercv, &peerco, &peercc))
		return (false);

	ConfigTypeAddressFamily *peer_familyct;
	ConfigValue *peer_familycv = peerco->get("family", &peer_familyct);
	if (peer_familycv == NULL)
		return (false);

	SocketAddressFamily peer_family;
	if (!peer_familyct->get(peer_familycv, &peer_family))
		return (false);

	ConfigTypeString *peer_hostct;
	ConfigValue *peer_hostcv = peerco->get("host", &peer_hostct);
	if (peer_hostcv == NULL)
		return (false);

	std::string peer_hoststr;
	if (!peer_hostct->get(peer_hostcv, &peer_hoststr))
		return (false);

	ConfigTypeString *peer_portct;
	ConfigValue *peer_portcv = peerco->get("port", &peer_portct);
	if (peer_portcv == NULL)
		return (false);

	std::string peer_portstr;
	if (!peer_portct->get(peer_portcv, &peer_portstr))
		return (false);

	/* Extract peer_codec.  */
	ConfigTypePointer *peer_codecct;
	ConfigValue *peer_codeccv = co->get("peer_codec", &peer_codecct);
	if (peer_codeccv == NULL)
		return (false);

	WANProxyConfigClassCodec *peer_codeccc;
	ConfigObject *peer_codecco;
	if (!peer_codecct->get(peer_codeccv, &peer_codecco, &peer_codeccc))
		return (false);

	WANProxyCodec *peer_codeccodec;
	if (peer_codecco != NULL) {
		peer_codeccodec = peer_codeccc->get(peer_codecco);
	} else {
		peer_codeccodec = NULL;
	}

	std::string interface_address = '[' + interface_hoststr + ']' + ':' + interface_portstr;
	std::string peer_address = '[' + peer_hoststr + ']' + ':' + peer_portstr;

	if (type == WANProxyConfigProxyTypeTCPTCP) {
		ProxyListener *listener = new ProxyListener(co->name(), interface_codeccodec, peer_codeccodec, interface_family, interface_address, peer_family, peer_address);
		object_listener_map_[co] = listener;
	} else {
		SSHProxyListener *listener = new SSHProxyListener(co->name(), interface_codeccodec, peer_codeccodec, interface_family, interface_address, peer_family, peer_address);
		(void)listener;
	}

	return (true);
}
