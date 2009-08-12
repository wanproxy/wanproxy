#include <config/config_class.h>
#include <config/config_class_log_mask.h>
#include <config/config_object.h>
#include <config/config_value.h>

ConfigClassLogMask config_class_log_mask;

bool
ConfigClassLogMask::activate(ConfigObject *co)
{
	ConfigTypeString *regexct;
	ConfigValue *regexcv = co->get("regex", &regexct);
	if (regexcv == NULL)
		return (false);

	std::string regex;
	if (!regexct->get(regexcv, &regex))
		return (false);

	ConfigTypeLogLevel *maskct;
	ConfigValue *maskcv = co->get("mask", &maskct);
	if (maskcv == NULL)
		return (false);

	Log::Priority priority;
	if (!maskct->get(maskcv, &priority))
		return (false);

	return (Log::mask(regex, priority));
}
