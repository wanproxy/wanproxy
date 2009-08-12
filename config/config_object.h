#ifndef	CONFIG_OBJECT_H
#define	CONFIG_OBJECT_H

#include <map>

#include <config/config_value.h>

class Config;
class ConfigClass;

class ConfigObject {
	friend class Config;
	friend class ConfigClass;

	Config *config_;
	ConfigClass *class_;
	std::map<std::string, ConfigValue *> members_;

public:
	ConfigObject(Config *config, ConfigClass *cc)
	: config_(config),
	  class_(cc),
	  members_()
	{ }

	~ConfigObject();

	template<typename T>
	T *coerce(void)
	{
		T *cc = dynamic_cast<T *>(class_);
		if (cc == NULL)
			return (NULL);
		return (cc);
	}

	template<typename T>
	ConfigValue *get(const std::string& name, T **ctp) const
	{
		std::map<std::string, ConfigValue *>::const_iterator it;

		it = members_.find(name);
		if (it == members_.end())
			return (NULL);

		ConfigValue *cv = it->second;
		if (cv == NULL)
			return (NULL);

		T *ct = dynamic_cast<T *>(cv->type_);
		if (ct == NULL)
			return (NULL);

		*ctp = ct;
		return (cv);
	}

	bool has(const std::string& name) const
	{
		std::map<std::string, ConfigValue *>::const_iterator it;

		it = members_.find(name);
		if (it == members_.end())
			return (false);
		return (true);
	}
};

#endif /* !CONFIG_OBJECT_H */
