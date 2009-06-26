#include <config/config_class.h>
#include <config/config_object.h>
#include <config/config_value.h>

#include "wanproxy_config_class_proxy_socks.h"

WANProxySocksConfigClassProxySocks wanproxy_config_class_proxy_socks;

bool
WANProxySocksConfigClassProxySocks::activate(ConfigObject *co)
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

	return (true);
}
