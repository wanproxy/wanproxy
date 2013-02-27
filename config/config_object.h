#ifndef	CONFIG_CONFIG_OBJECT_H
#define	CONFIG_CONFIG_OBJECT_H

#include <map>

class Config;
class ConfigClass;
class ConfigClassInstance;
class ConfigExporter;

struct ConfigObject {
	Config *config_;
	std::string name_;
	const ConfigClass *class_;
	ConfigClassInstance *instance_;
	std::map<std::string, std::string> member_values_;

	ConfigObject(Config *config, const std::string& name, const ConfigClass *cc, ConfigClassInstance *inst)
	: config_(config),
	  name_(name),
	  class_(cc),
	  instance_(inst),
	  member_values_()
	{ }

	virtual ~ConfigObject()
	{
		/* XXX delete instance_ ? */
	}

	bool activate(void) const;
	void marshall(ConfigExporter *) const;
	bool set(const std::string&, const std::string&);
};

#endif /* !CONFIG_CONFIG_OBJECT_H */
