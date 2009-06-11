#include <string>

#include <config/config_class.h>
#include <config/config_object.h>
#include <config/config_type.h>
#include <config/config_value.h>

ConfigClass::~ConfigClass()
{
	std::map<std::string, ConfigType *>::iterator it;

	while ((it = members_.begin()) != members_.end())
		members_.erase(it);
}

bool
ConfigClass::set(ConfigObject *co, const std::string& mname, ConfigValue *cv)
{
	if (co->class_ != this)
		return (false);

	ConfigType *type = member(mname);
	if (type == NULL || type != cv->type_)
		return (false);

	if (co->members_.find(mname) != co->members_.end())
		return (false);
	co->members_[mname] = cv;

	return (true);
}
