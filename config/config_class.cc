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
ConfigClass::set(ConfigObject *co, const std::string& mname, ConfigType *ct, const std::string& vstr)
{
	ASSERT(co->class_ == this);

	ConfigType *type = member(mname);
	if (type == NULL) {
		ERROR("/config/class") << "Object member (" << mname << ") does not exist.";
		return (false);
	}
	if (type != ct) {
		ERROR("/config/class") << "Object member (" << mname << ") has wrong type.";
		return (false);
	}

	if (co->members_.find(mname) != co->members_.end()) {
		ConfigValue *cv = co->members_[mname];
		if (!ct->set(cv, vstr)) {
			ERROR("/config/class") << "Member (" << mname << ") could not be set again.";
			return (false);
		}
	} else {
		ConfigValue *cv = new ConfigValue(co->config_, ct, vstr);
		if (!ct->set(cv, vstr)) {
			delete cv;
			ERROR("/config/class") << "Member (" << mname << ") could not be set.";
			return (false);
		}
		co->members_[mname] = cv;
	}

	return (true);
}
