#include <config/config_class.h>
#include <config/config_class_address.h>
#include <config/config_object.h>
#include <config/config_value.h>

ConfigClassAddress config_class_address;

bool
ConfigClassAddress::activate(ConfigObject *co)
{
	ConfigValue *familycv = co->members_["family"];
	if (familycv == NULL)
		return (false);

	ConfigTypeAddressFamily *familyct = dynamic_cast<ConfigTypeAddressFamily *>(familycv->type_);
	if (familyct == NULL)
		return (false);

	ConfigAddressFamily family;
	if (!familyct->get(familycv, &family))
		return (false);

	if (family == ConfigAddressFamilyIPv4) {
		if (co->members_["path"] != NULL)
			return (false);
		return (true);
	}

	if (family == ConfigAddressFamilyUnix) {
		if (co->members_["host"] != NULL || co->members_["port"] != NULL)
			return (false);
		return (true);
	}

	return (false);
}
