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
	ASSERT(co->class_ == this);

	ConfigType *type = member(mname);
	if (type == NULL) {
		ERROR("/config/class") << "Object member (" << mname << ") does not exist.";
		return (false);
	}
	if (type != cv->type_) {
		ERROR("/config/class") << "Object member (" << mname << ") has wrong type.";
		return (false);
	}

	if (co->members_.find(mname) != co->members_.end()) {
		ERROR("/config/class") << "Object member (" << mname << ") already set.";
		return (false);
	}
	co->members_[mname] = cv;

	return (true);
}
