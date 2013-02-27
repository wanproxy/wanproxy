#include <config/config_class.h>
#include <config/config_class_log_mask.h>

ConfigClassLogMask config_class_log_mask;

bool
ConfigClassLogMask::Instance::activate(const ConfigObject *)
{
	if (!Log::mask(regex_, mask_)) {
		ERROR("/config/class/logmask") << "Could not set log mask.";
		return (false);
	}

	return (true);
}
