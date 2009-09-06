#if !defined(__OPENNT)
#include <inttypes.h>
#endif
#include <stdlib.h>

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

#if !defined(__OPENNT)
	imax = strtoimax(str, &endp, 0);
#else
	imax = strtoll(str, &endp, 0);
#endif
	if (*endp != '\0')
		return (false);

	ints_[cv] = imax;

	return (true);
}
