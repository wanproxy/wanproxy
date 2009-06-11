#ifndef	CONFIG_OBJECT_H
#define	CONFIG_OBJECT_H

#include <map>

class ConfigClass;
class ConfigValue;

struct ConfigObject {
	ConfigClass *class_;
	std::map<std::string, ConfigValue *> members_;

	ConfigObject(ConfigClass *cc)
	: class_(cc),
	  members_()
	{ }

	~ConfigObject();
};

#endif /* !CONFIG_OBJECT_H */
