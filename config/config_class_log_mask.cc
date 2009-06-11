#include <string>

#include <config/config_class.h>
#include <config/config_class_log_mask.h>
#include <config/config_object.h>
#include <config/config_value.h>

ConfigClassLogMask config_class_log_mask;

bool
ConfigClassLogMask::activate(ConfigObject *co)
{
	ConfigValue *regexcv = co->members_["regex"];
	if (regexcv == NULL)
		return (false);

	ConfigTypeString *regexct = dynamic_cast<ConfigTypeString *>(regexcv->type_);
	if (regexct == NULL)
		return (false);

	std::string regex;
	if (!regexct->get(regexcv, &regex))
		return (false);

	/* XXX enum */
	ConfigValue *maskcv = co->members_["mask"];
	if (maskcv == NULL)
		return (false);

	ConfigTypeString *maskct = dynamic_cast<ConfigTypeString *>(maskcv->type_);
	if (maskct == NULL)
		return (false);

	std::string mask;
	if (!maskct->get(maskcv, &mask))
		return (false);

	return (Log::mask(regex, mask));
}
