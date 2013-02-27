#ifndef	CONFIG_CONFIG_TYPE_POINTER_H
#define	CONFIG_CONFIG_TYPE_POINTER_H

#include <map>

#include <config/config_type.h>

struct ConfigValue;

class ConfigTypePointer : public ConfigType {
public:
	ConfigTypePointer(void)
	: ConfigType("pointer")
	{ }

	~ConfigTypePointer()
	{ }

	void marshall(ConfigExporter *, ConfigObject *const *) const;

	bool set(ConfigObject *, const std::string&, ConfigObject **);
};

extern ConfigTypePointer config_type_pointer;

#endif /* !CONFIG_CONFIG_TYPE_POINTER_H */
