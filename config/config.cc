#include <config/config.h>
#include <config/config_class.h>
#include <config/config_exporter.h>
#include <config/config_object.h>
#include <config/config_type.h>

bool
Config::activate(const std::string& oname)
{
	if (object_map_.find(oname) == object_map_.end()) {
		ERROR(log_) << "Object (" << oname << ") can not be activated as it does not exist.";
		return (false);
	}

	ConfigObject *co = object_map_[oname];
	return (co->activate());
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

	object_map_[oname] = new ConfigObject(this, oname, cc, cc->allocate());

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

#if 0
	ConfigType *ct = cc->member(mname);
	if (ct == NULL) {
		ERROR(log_) << "No such member (" << mname << ") in object (" << oname << ")";
		return (false);
	}
#endif

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

		object_field_string_map_t::const_iterator fsit;
		fsit = field_strings_map_.find(object_field_string_map_t::key_type(oco, omname));
		if (fsit == field_strings_map_.end()) {
			ERROR(log_) << "Reference to unset member (" << omname << ") in object (" << ooname << ")";
			return (false);
		}

		return (set(oname, mname, fsit->second));
	}

	if (!co->set(mname, vstr)) {
		ERROR(log_) << "Member (" << mname << ") in object (" << oname << ") could not be set.";
		return (false);
	}

	field_strings_map_[object_field_string_map_t::key_type(co, mname)] = vstr;

	return (true);
}

void
Config::import(ConfigClass *cc)
{
	if (class_map_.find(cc->name_) != class_map_.end())
		return;

	class_map_[cc->name_] = cc;
}

void
Config::marshall(ConfigExporter *exp) const
{
	std::map<std::string, ConfigObject *>::const_iterator it;

	for (it = object_map_.begin(); it != object_map_.end(); ++it)
		exp->object(it->second, it->first);
}
