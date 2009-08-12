#include <common/buffer.h>

#include <config/config_class.h>
#include <config/config_object.h>
#include <config/config_value.h>

#include <event/action.h>
#include <event/event.h>

#include "proxy_listener.h"
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

	/* Extract interface.  */
	ConfigTypePointer *interfacect;
	ConfigValue *interfacecv = co->get("interface", &interfacect);
	if (interfacecv == NULL)
		return (false);

	WANProxyConfigClassInterface *interfacecc;
	ConfigObject *interfaceco;
	if (!interfacect->get(interfacecv, &interfaceco, &interfacecc))
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

	/* Extract decoder.  */
	ConfigTypePointer *decoderct;
	ConfigValue *decodercv = co->get("decoder", &decoderct);
	if (decodercv == NULL)
		return (false);

	WANProxyConfigClassCodec *decodercc;
	ConfigObject *decoderco;
	if (!decoderct->get(decodercv, &decoderco, &decodercc))
		return (false);

	XCodec *decodercodec;
	if (decoderco != NULL) {
		decodercodec = decodercc->get(decoderco);
	} else {
		decodercodec = NULL;
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

	/* Extract encoder.  */
	ConfigTypePointer *encoderct;
	ConfigValue *encodercv = co->get("encoder", &encoderct);
	if (encodercv == NULL)
		return (false);

	WANProxyConfigClassCodec *encodercc;
	ConfigObject *encoderco;
	if (!encoderct->get(encodercv, &encoderco, &encodercc))
		return (false);

	XCodec *encodercodec;
	if (encoderco != NULL) {
		encodercodec = encodercc->get(encoderco);
	} else {
		encodercodec = NULL;
	}

	std::string interface_address = interface_hoststr + ':' + interface_portstr;
	std::string peer_address = peer_hoststr + ':' + peer_portstr;

	ProxyListener *listener = new ProxyListener(decodercodec, encodercodec, interface_address, peer_address);
	object_listener_map_[co] = listener;

	return (true);
}
