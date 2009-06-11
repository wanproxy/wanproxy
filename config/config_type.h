#ifndef	CONFIG_TYPE_H
#define	CONFIG_TYPE_H

class ConfigType {
	std::string name_;
protected:
	ConfigType(const std::string& name)
	: name_(name)
	{ }

	~ConfigType()
	{ }
public:
	virtual bool set(ConfigValue *, const std::string&) = 0;
};

#endif /* !CONFIG_TYPE_H */
