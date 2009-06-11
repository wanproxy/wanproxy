#include <string>

#include <config/config.h>
#include <config/config_class.h>
#include <config/config_object.h>
#include <config/config_type.h>
#include <config/config_value.h>

Config::Config(void)
: log_("/config"),
  class_map_(),
  object_map_()
{ }

Config::~Config()
{
	ASSERT(class_map_.empty());
	ASSERT(object_map_.empty());
}

bool
Config::activate(const std::string& oname)
{
	if (object_map_.find(oname) == object_map_.end()) {
		ERROR(log_) << "Object (" << oname << ") can not be activated as it does not exist.";
		return (false);
	}

	ConfigObject *co = object_map_[oname];
	ConfigClass *cc = co->class_;

	return (cc->activate(co));
}

bool
Config::create(const std::string& cname, const std::string& oname)
{
	if (class_map_.find(cname) == class_map_.end()) {
		ERROR(log_) << "Class (" << cname << ") not present.";
		return (false);
	}
	if (object_map_.find(oname) != object_map_.end()) {
		ERROR(log_) << "Refusing to create object (" << oname << ") twice.";
		return (false);
	}

	ConfigClass *cc = class_map_[cname];
	ConfigObject *co = new ConfigObject(this, cc);

	object_map_[oname] = co;

	return (true);
}

bool
Config::set(const std::string& oname, const std::string& mname,
	    const std::string& vstr)
{
	if (object_map_.find(oname) == object_map_.end()) {
		ERROR(log_) << "Can not set member value on object (" << oname << ") that does not exist.";
		return (false);
	}

	ConfigObject *co = object_map_[oname];
	ConfigClass *cc = co->class_;

	ConfigType *ct = cc->member(mname);
	if (ct == NULL) {
		ERROR(log_) << "No such member (" << mname << ") in object (" << oname << ")";
		return (false);
	}

	ConfigValue *cv = new ConfigValue(this, ct);
	if (!ct->set(cv, vstr)) {
		delete cv;
		return (false);
	}
	cc->set(co, mname, cv);

	return (true);
}

void
Config::import(const std::string& cname, ConfigClass *cc)
{
	if (class_map_.find(cname) != class_map_.end())
		return;

	class_map_[cname] = cc;
}
