#include <string>

#include <config/config.h>

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

	DEBUG(log_) << __PRETTY_FUNCTION__ << ", oname=\"" << oname << "\"";
	return (true);
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

	DEBUG(log_) << __PRETTY_FUNCTION__ << ", cname=\"" << cname << "\", oname=\"" << oname << "\"";
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

	DEBUG(log_) << __PRETTY_FUNCTION__ << ", oname=\"" << oname << "\", mname=\"" << mname << "\"" << ", vstr=\"" << vstr << "\"";
	return (true);
}

void
Config::import(const std::string& cname, ConfigClass *cc)
{
	if (class_map_.find(cname) != class_map_.end())
		return;

	class_map_[cname] = cc;
}
