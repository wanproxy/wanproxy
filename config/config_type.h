#ifndef	CONFIG_TYPE_H
#define	CONFIG_TYPE_H

class ConfigExporter;
struct ConfigValue;

class ConfigType {
	std::string name_;
protected:
	ConfigType(const std::string& name)
	: name_(name)
	{ }

	virtual ~ConfigType()
	{ }
public:
	virtual void marshall(ConfigExporter *, const ConfigValue *) const = 0;
	virtual bool set(const ConfigValue *, const std::string&) = 0;
};

#endif /* !CONFIG_TYPE_H */
