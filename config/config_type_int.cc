#include <inttypes.h>
#include <stdlib.h>

#include <string>

#include <config/config_type_int.h>

ConfigTypeInt config_type_int;

bool
ConfigTypeInt::set(ConfigValue *cv, const std::string& vstr)
{
	if (ints_.find(cv) != ints_.end())
		return (false);

	const char *str = vstr.c_str();
	char *endp;
	intmax_t imax;

	imax = strtoimax(str, &endp, 0);
	if (*endp != '\0')
		return (false);

	ints_[cv] = imax;

	return (true);
}
