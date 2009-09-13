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
#if 0 /* XXX NOTYET */
	ASSERT(class_map_.empty());
	ASSERT(object_map_.empty());
#endif
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

	if (vstr[0] == '$') {
		std::string::const_iterator dot;

		dot = std::find(vstr.begin(), vstr.end(), '.');

		std::string ooname(vstr.begin() + 1, dot);
		std::string omname(dot + 1, vstr.end());

		if (ooname == "" || omname == "") {
			ERROR(log_) << "Refernece to invalid object (" << ooname << ") or member (" << omname << ") by name.";
			return (false);
		}

		if (object_map_.find(ooname) == object_map_.end()) {
			ERROR(log_) << "Reference to non-existant object (" << ooname << ")";
			return (false);
		}

		ConfigObject *oco = object_map_[ooname];
		ConfigClass *occ = oco->class_;

		ConfigType *oct = occ->member(mname);
		if (oct == NULL) {
			ERROR(log_) << "Reference to non-existant member (" << omname << ") in object (" << ooname << ")";
			return (false);
		}

		ConfigValue *ocv = oco->members_[omname];
		if (ocv == NULL) {
			ERROR(log_) << "Reference to uninitialized member (" << omname << ") in object (" << ooname << ")";
			return (false);
		}

		return (set(oname, mname, ocv->string_));
	}

	if (!cc->set(co, mname, ct, vstr)) {
		ERROR(log_) << "Member (" << mname << ") in object (" << oname << ") could not be set.";
		return (false);
	}

	return (true);
}

void
Config::import(ConfigClass *cc)
{
	if (class_map_.find(cc->name_) != class_map_.end())
		return;

	class_map_[cc->name_] = cc;
}
