#include <config/config_class.h>
#include <config/config_exporter.h>
#include <config/config_type.h>
#include <config/config_value.h>

void
ConfigValue::marshall(ConfigExporter *exp) const
{
	type_->marshall(exp, this);
}
