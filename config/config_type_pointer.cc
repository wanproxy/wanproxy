#include <stdio.h>

#include <config/config.h>
#include <config/config_class.h>
#include <config/config_exporter.h>
#include <config/config_object.h>
#include <config/config_type_pointer.h>

ConfigTypePointer config_type_pointer;

void
ConfigTypePointer::marshall(ConfigExporter *exp, ConfigObject *const *cop) const
{
	const ConfigObject *co = *cop;

	if (co == NULL)
		exp->value(this, "None");
	else
		exp->value(this, co->name_);
}

bool
ConfigTypePointer::set(ConfigObject *co, const std::string& vstr, ConfigObject **cop)
{
	if (vstr == "None") {
		*cop = NULL;
		return (true);
	}

	ConfigObject *target = co->config_->lookup(vstr);
	if (target == NULL) {
		ERROR("/config/type/pointer") << "Referenced object (" << vstr << ") does not exist.";
		return (false);
	}

	*cop = target;

	return (true);
}
