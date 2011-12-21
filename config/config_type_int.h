#ifndef	CONFIG_CONFIG_TYPE_INT_H
#define	CONFIG_CONFIG_TYPE_INT_H

#include <map>

#include <config/config_type.h>

struct ConfigValue;

class ConfigTypeInt : public ConfigType {
	std::map<const ConfigValue *, intmax_t> ints_;
public:
	ConfigTypeInt(void)
	: ConfigType("int"),
	  ints_()
	{ }

	~ConfigTypeInt()
	{
		ints_.clear();
	}

	bool get(const ConfigValue *cv, intmax_t *intp) const
	{
		std::map<const ConfigValue *, intmax_t>::const_iterator it;
		it = ints_.find(cv);
		if (it == ints_.end()) {
			ERROR("/config/type/int") << "Value not set.";
			return (false);
		}
		*intp = it->second;
		return (true);
	}

	void marshall(ConfigExporter *, const ConfigValue *) const;

	bool set(const ConfigValue *, const std::string&);
};

extern ConfigTypeInt config_type_int;

#endif /* !CONFIG_CONFIG_TYPE_INT_H */
