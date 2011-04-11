#ifndef	CONFIG_VALUE_H
#define	CONFIG_VALUE_H

class Config;
class ConfigExporter;
class ConfigType;

struct ConfigValue {
	Config *config_;
	ConfigType *type_;
	std::string string_;

	ConfigValue(Config *config, ConfigType *type, const std::string& string)
	: config_(config),
	  type_(type),
	  string_(string)
	{ }

	~ConfigValue()
	{ }

	void marshall(ConfigExporter *) const;
};

#endif /* !CONFIG_VALUE_H */
