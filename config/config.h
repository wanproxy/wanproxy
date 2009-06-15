#ifndef	CONFIG_H
#define	CONFIG_H

#include <map>

class ConfigClass;
class ConfigObject;

class Config {
	LogHandle log_;
	std::map<std::string, ConfigClass *> class_map_;
	std::map<std::string, ConfigObject *> object_map_;

public:
	Config(void);
	~Config();

	bool activate(const std::string&);
	bool create(const std::string&, const std::string&);
	bool set(const std::string&, const std::string&, const std::string&);

	void import(ConfigClass *);

	ConfigObject *lookup(const std::string& oname) const
	{
		std::map<std::string, ConfigObject *>::const_iterator omit;

		omit = object_map_.find(oname);
		if (omit == object_map_.end())
			return (NULL);
		return (omit->second);
	}
};

#endif /* !CONFIG_H */
