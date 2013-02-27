#if !defined(__OPENNT)
#include <inttypes.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include <config/config_exporter.h>
#include <config/config_type_int.h>

ConfigTypeInt config_type_int;

void
ConfigTypeInt::marshall(ConfigExporter *exp, const intmax_t *valp) const
{
	intmax_t val = *valp;

	char buf[sizeof val * 2 + 2 + 1];
	snprintf(buf, sizeof buf, "0x%016jx", val);
	exp->value(this, buf);
}

bool
ConfigTypeInt::set(ConfigObject *, const std::string& vstr, intmax_t *valp)
{
	const char *str = vstr.c_str();
	char *endp;
	intmax_t imax;

#if !defined(__OPENNT)
	imax = strtoimax(str, &endp, 0);
#else
	imax = strtoll(str, &endp, 0);
#endif
	if (*endp != '\0') {
		ERROR("/config/type/int") << "Invalid numeric format.";
		return (false);
	}

	*valp = imax;

	return (true);
}
