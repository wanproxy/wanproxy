#include <string>

#include <config/config.h>
#include <config/config_type_pointer.h>
#include <config/config_value.h>

ConfigTypePointer config_type_pointer;

bool
ConfigTypePointer::set(ConfigValue *cv, const std::string& vstr)
{
	if (pointers_.find(cv) != pointers_.end())
		return (false);

	Config *config = cv->config_;
	ConfigObject *co = config->lookup(vstr);
	if (co == NULL)
		return (false);

	pointers_[cv] = co;

	return (true);
}
