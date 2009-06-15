#ifndef	CONFIG_TYPE_POINTER_H
#define	CONFIG_TYPE_POINTER_H

#include <map>

#include <config/config_type.h>

class ConfigValue;

class ConfigTypePointer : public ConfigType {
	std::map<ConfigValue *, ConfigObject *> pointers_;
public:
	ConfigTypePointer(void)
	: ConfigType("pointer"),
	  pointers_()
	{ }

	~ConfigTypePointer()
	{
		pointers_.clear();
	}

	bool get(ConfigValue *cv, ConfigObject **cop)
	{
		if (pointers_.find(cv) == pointers_.end())
			return (false);
		*cop = pointers_[cv];
		return (true);
	}

	bool set(ConfigValue *, const std::string&);
};

extern ConfigTypePointer config_type_pointer;

#endif /* !CONFIG_TYPE_POINTER_H */
