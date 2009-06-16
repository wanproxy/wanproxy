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

	ConfigValue *maskcv = co->members_["mask"];
	if (maskcv == NULL)
		return (false);

	ConfigTypeLogLevel *maskct = dynamic_cast<ConfigTypeLogLevel *>(maskcv->type_);
	if (maskct == NULL)
		return (false);

	Log::Priority priority;
	if (!maskct->get(maskcv, &priority))
		return (false);

	return (Log::mask(regex, priority));
}
