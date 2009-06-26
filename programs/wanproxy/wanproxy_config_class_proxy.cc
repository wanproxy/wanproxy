#include <config/config_class.h>
#include <config/config_object.h>
#include <config/config_value.h>

#include "wanproxy_config_class_proxy.h"

WANProxyConfigClassProxy wanproxy_config_class_proxy;

bool
WANProxyConfigClassProxy::activate(ConfigObject *co)
{
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

	return (true);
}
