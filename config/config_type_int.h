#ifndef	CONFIG_TYPE_INT_H
#define	CONFIG_TYPE_INT_H

#include <map>

#include <config/config_type.h>

class ConfigValue;

class ConfigTypeInt : public ConfigType {
	std::map<ConfigValue *, intmax_t> ints_;
public:
	ConfigTypeInt(void)
	: ConfigType("int"),
	  ints_()
	{ }

	~ConfigTypeInt()
	{
		ints_.clear();
	}

	bool get(ConfigValue *cv, intmax_t *intp)
	{
		if (ints_.find(cv) == ints_.end())
			return (false);
		*intp = ints_[cv];
		return (true);
	}

	bool set(ConfigValue *cv, const std::string& vstr);
};

extern ConfigTypeInt config_type_int;

#endif /* !CONFIG_TYPE_INT_H */
