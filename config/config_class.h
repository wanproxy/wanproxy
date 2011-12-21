#ifndef	CONFIG_CONFIG_CLASS_H
#define	CONFIG_CONFIG_CLASS_H

#include <map>

class Config;
class ConfigExporter;
class ConfigObject;
class ConfigType;
struct ConfigValue;

class ConfigClass {
	friend class Config;

	std::string name_;
	std::map<std::string, ConfigType *> members_;
protected:
	ConfigClass(const std::string& xname)
	: name_(xname),
	  members_()
	{ }

	virtual ~ConfigClass();

protected:
	void add_member(const std::string& mname, ConfigType *type)
	{
		ASSERT("/config/class/" + name_, members_.find(mname) == members_.end());
		members_[mname] = type;
	}

private:
	virtual bool activate(ConfigObject *)
	{
		return (true);
	}

	bool set(ConfigObject *, const std::string&, ConfigType *, const std::string&);

	ConfigType *member(const std::string& mname) const
	{
		std::map<std::string, ConfigType *>::const_iterator it;

		it = members_.find(mname);
		if (it == members_.end())
			return (NULL);
		return (it->second);
	}

public:
	void marshall(ConfigExporter *, const ConfigObject *) const;

	std::string name(void) const
	{
		return (name_);
	}
};

#endif /* !CONFIG_CONFIG_CLASS_H */
