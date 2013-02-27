#ifndef	CONFIG_CONFIG_TYPE_INT_H
#define	CONFIG_CONFIG_TYPE_INT_H

#include <map>

#include <config/config_type.h>

class ConfigTypeInt : public ConfigType {
public:
	ConfigTypeInt(void)
	: ConfigType("int")
	{ }

	~ConfigTypeInt()
	{ }

	void marshall(ConfigExporter *, const intmax_t *) const;

	bool set(ConfigObject *, const std::string&, intmax_t *);
};

extern ConfigTypeInt config_type_int;

#endif /* !CONFIG_CONFIG_TYPE_INT_H */
