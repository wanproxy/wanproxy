#ifndef	CONFIG_CONFIG_TYPE_H
#define	CONFIG_CONFIG_TYPE_H

class Config;
class ConfigExporter;

class ConfigType {
	std::string name_;
protected:
	ConfigType(const std::string& xname)
	: name_(xname)
	{ }

	virtual ~ConfigType()
	{ }
public:
	std::string name(void) const
	{
		return (name_);
	}
};

#endif /* !CONFIG_CONFIG_TYPE_H */
