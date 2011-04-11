#include <config/config.h>
#include <config/config_exporter.h>
#include <config/config_type_pointer.h>
#include <config/config_value.h>

ConfigTypePointer config_type_pointer;

void
ConfigTypePointer::marshall(ConfigExporter *exp, const ConfigValue *cv) const
{
	std::map<const ConfigValue *, ConfigObject *>::const_iterator it;

	it = pointers_.find(cv);
	if (it == pointers_.end())
		HALT("/config/type/pointer") << "Trying to marshall unset value.";

	const ConfigObject *co = it->second;

	char buf[sizeof co * 2 + 2 + 1];
	snprintf(buf, sizeof buf, "%p", co);
	exp->value(cv, buf);
}

bool
ConfigTypePointer::set(const ConfigValue *cv, const std::string& vstr)
{
	if (pointers_.find(cv) != pointers_.end()) {
		ERROR("/config/type/pointer") << "Value already set.";
		return (false);
	}

	/* XXX Have a magic None that is easily-detected?  */
	if (vstr == "None") {
		pointers_[cv] = NULL;
		return (true);
	}

	Config *config = cv->config_;
	ConfigObject *co = config->lookup(vstr);
	if (co == NULL) {
		ERROR("/config/type/pointer") << "Referenced object (" << vstr << ") does not exist.";
		return (false);
	}

	pointers_[cv] = co;

	return (true);
}
