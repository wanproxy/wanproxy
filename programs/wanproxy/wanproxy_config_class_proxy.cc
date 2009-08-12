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

	ConfigTypeString *interface_portct = dynamic_cast<ConfigTypeString *>(interface_portcv->type_);
	if (interface_portct == NULL)
		return (false);

	std::string interface_portstr;
	if (!interface_portct->get(interface_portcv, &interface_portstr))
		return (false);

	/* Extract decoder.  */
	ConfigValue *decodercv = co->members_["decoder"];
	if (decodercv == NULL)
		return (false);

	ConfigTypePointer *decoderct = dynamic_cast<ConfigTypePointer *>(decodercv->type_);
	if (decoderct == NULL)
		return (false);

	ConfigObject *decoderco;
	if (!decoderct->get(decodercv, &decoderco))
		return (false);

	XCodec *decodercodec;
	if (decoderco != NULL) {
		WANProxyConfigClassCodec *decodercc = dynamic_cast<WANProxyConfigClassCodec *>(decoderco->class_);
		if (decodercc == NULL)
			return (false);
		decodercodec = decodercc->get(decoderco);
	} else {
		decodercodec = NULL;
	}

	/* Extract peer.  */
	ConfigValue *peercv = co->members_["peer"];
	if (peercv == NULL)
		return (false);

	ConfigTypePointer *peerct = dynamic_cast<ConfigTypePointer *>(peercv->type_);
	if (peerct == NULL)
		return (false);

	ConfigObject *peerco;
	if (!peerct->get(peercv, &peerco))
		return (false);

	WANProxyConfigClassPeer *peercc = dynamic_cast<WANProxyConfigClassPeer *>(peerco->class_);
	if (peercc == NULL)
		return (false);

	ConfigValue *peer_hostcv = peerco->members_["host"];
	if (peer_hostcv == NULL)
		return (false);

	ConfigTypeString *peer_hostct = dynamic_cast<ConfigTypeString *>(peer_hostcv->type_);
	if (peer_hostct == NULL)
		return (false);

	std::string peer_hoststr;
	if (!peer_hostct->get(peer_hostcv, &peer_hoststr))
		return (false);

	ConfigValue *peer_portcv = peerco->members_["port"];
	if (peer_portcv == NULL)
		return (false);

	ConfigTypeString *peer_portct = dynamic_cast<ConfigTypeString *>(peer_portcv->type_);
	if (peer_portct == NULL)
		return (false);

	std::string peer_portstr;
	if (!peer_portct->get(peer_portcv, &peer_portstr))
		return (false);

	/* Extract encoder.  */
	ConfigValue *encodercv = co->members_["encoder"];
	if (encodercv == NULL)
		return (false);

	ConfigTypePointer *encoderct = dynamic_cast<ConfigTypePointer *>(encodercv->type_);
	if (encoderct == NULL)
		return (false);

	ConfigObject *encoderco;
	if (!encoderct->get(encodercv, &encoderco))
		return (false);

	XCodec *encodercodec;
	if (encoderco != NULL) {
		WANProxyConfigClassCodec *encodercc = dynamic_cast<WANProxyConfigClassCodec *>(encoderco->class_);
		if (encodercc == NULL)
			return (false);
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
