#include <config/config_class.h>
#include <config/config_exporter.h>
#include <config/config_object.h>

bool
ConfigObject::activate(void) const
{
	return (instance_->activate(this));
}

void
ConfigObject::marshall(ConfigExporter *exp) const
{
	class_->marshall(exp, instance_);
}

bool
ConfigObject::set(const std::string& mname, const std::string& vstr)
{
	bool ok = class_->set(this, mname, vstr);
	if (!ok)
		return (false);
	member_values_[mname] = vstr;
	return (true);
}
