#include <config/config_class.h>
#include <config/config_exporter.h>
#include <config/config_type.h>

ConfigClass::~ConfigClass()
{
	std::map<std::string, const ConfigClassMember *>::iterator it;

	while ((it = members_.begin()) != members_.end()) {
		delete it->second;
		members_.erase(it);
	}
}

bool
ConfigClass::set(ConfigObject *co, const std::string& mname, const std::string& vstr) const
{
#if 0
	ASSERT("/config/class", co->class_ == this);
#endif

	std::map<std::string, const ConfigClassMember *>::const_iterator it = members_.find(mname);
	if (it == members_.end()) {
		ERROR("/config/class") << "Object member (" << mname << ") does not exist.";
		return (false);
	}
	return (it->second->set(co, vstr));
}

void
ConfigClass::marshall(ConfigExporter *exp, const ConfigClassInstance *co) const
{
	std::map<std::string, const ConfigClassMember *>::const_iterator it;
	for (it = members_.begin(); it != members_.end(); ++it) {
		exp->field(co, it->second, it->first);
	}
}
