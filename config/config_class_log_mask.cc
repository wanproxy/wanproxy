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
	if (regexcv == NULL) {
		ERROR("/config/class/logmask") << "Could not get regex.";
		return (false);
	}

	std::string regex;
	if (!regexct->get(regexcv, &regex)) {
		ERROR("/config/class/logmask") << "Could not get regex.";
		return (false);
	}

	ConfigTypeLogLevel *maskct;
	ConfigValue *maskcv = co->get("mask", &maskct);
	if (maskcv == NULL) {
		ERROR("/config/class/logmask") << "Could not get log level mask.";
		return (false);
	}

	Log::Priority priority;
	if (!maskct->get(maskcv, &priority)) {
		ERROR("/config/class/logmask") << "Could not get log level mask.";
		return (false);
	}

	if (!Log::mask(regex, priority)) {
		ERROR("/config/class/logmask") << "Could not set log mask.";
		return (false);
	}

	return (true);
}
