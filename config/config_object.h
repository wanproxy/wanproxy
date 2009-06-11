#ifndef	CONFIG_OBJECT_H
#define	CONFIG_OBJECT_H

#include <map>

class Config;
class ConfigClass;
class ConfigValue;

struct ConfigObject {
	Config *config_;
	ConfigClass *class_;
	std::map<std::string, ConfigValue *> members_;

	ConfigObject(Config *config, ConfigClass *cc)
	: config_(config),
	  class_(cc),
	  members_()
	{ }

	~ConfigObject();
};

#endif /* !CONFIG_OBJECT_H */
