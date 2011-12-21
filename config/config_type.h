#ifndef	CONFIG_CONFIG_TYPE_H
#define	CONFIG_CONFIG_TYPE_H

class ConfigExporter;
struct ConfigValue;

class ConfigType {
	std::string name_;
protected:
	ConfigType(const std::string& xname)
	: name_(xname)
	{ }

	virtual ~ConfigType()
	{ }
public:
	virtual void marshall(ConfigExporter *, const ConfigValue *) const = 0;
	virtual bool set(const ConfigValue *, const std::string&) = 0;

	std::string name(void) const
	{
		return (name_);
	}
};

#endif /* !CONFIG_CONFIG_TYPE_H */
