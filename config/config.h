#ifndef	CONFIG_CONFIG_H
#define	CONFIG_CONFIG_H

#include <map>

class ConfigClass;
class ConfigClassInstance;
class ConfigExporter;
struct ConfigObject;

class Config {
	LogHandle log_;
	std::map<std::string, ConfigClass *> class_map_;
	std::map<std::string, ConfigObject *> object_map_;

public:
	Config(void)
	: log_("/config"),
	  class_map_(),
	  object_map_()
	{ }

	~Config()
	{
#if 0 /* XXX NOTYET */
		ASSERT(log_, class_map_.empty());
		ASSERT(log_, object_map_.empty());
#endif
	}

	bool activate(const std::string&);
	bool create(const std::string&, const std::string&);
	bool set(const std::string&, const std::string&, const std::string&);

	void import(ConfigClass *);

	void marshall(ConfigExporter *) const;

	ConfigObject *lookup(const std::string& oname) const
	{
		std::map<std::string, ConfigObject *>::const_iterator omit;

		omit = object_map_.find(oname);
		if (omit == object_map_.end())
			return (NULL);
		return (omit->second);
	}
};

#endif /* !CONFIG_CONFIG_H */
