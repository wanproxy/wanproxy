#ifndef	CONFIG_VALUE_H
#define	CONFIG_VALUE_H

class ConfigType;

struct ConfigValue {
	ConfigType *type_;

	ConfigValue(ConfigType *type)
	: type_(type)
	{ }

	~ConfigValue()
	{ }
};

#endif /* !CONFIG_VALUE_H */
