#ifndef	CONFIG_CLASS_H
#define	CONFIG_CLASS_H

#include <map>

class Config;
class ConfigObject;
class ConfigType;
class ConfigValue;

class ConfigClass {
	friend class Config;

	std::string name_;
	std::map<std::string, ConfigType *> members_;
protected:
	ConfigClass(const std::string& name)
	: name_(name),
	  members_()
	{ }

	~ConfigClass();

protected:
	void add_member(const std::string& mname, ConfigType *type)
	{
		ASSERT(members_.find(mname) == members_.end());
		members_[mname] = type;
	}

private:
	virtual bool activate(ConfigObject *)
	{
		return (true);
	}

	bool set(ConfigObject *, const std::string&, ConfigValue *);

	ConfigType *member(const std::string& mname) const
	{
		std::map<std::string, ConfigType *>::const_iterator it;

		it = members_.find(mname);
		if (it == members_.end())
			return (NULL);
		return (it->second);
	}
};

#endif /* !CONFIG_CLASS_H */
