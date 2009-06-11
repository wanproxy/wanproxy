#include <string>

#include <config/config_object.h>
#include <config/config_value.h>

ConfigObject::~ConfigObject()
{
	std::map<std::string, ConfigValue *>::iterator it;

	while ((it = members_.begin()) != members_.end()) {
		ConfigValue *cv = it->second;

		members_.erase(it);
		delete cv;
	}
}
