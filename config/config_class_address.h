#ifndef	CONFIG_CLASS_ADDRESS_H
#define	CONFIG_CLASS_ADDRESS_H

#include <config/config_type_address_family.h>
#include <config/config_type_string.h>

class ConfigObject;

class ConfigClassAddress : public ConfigClass {
public:
	ConfigClassAddress(const std::string& name = "address")
	: ConfigClass(name)
	{
		add_member("family", &config_type_address_family);
		add_member("host", &config_type_string);
		add_member("port", &config_type_string); /* XXX enum?  */
		add_member("path", &config_type_string);
	}

	~ConfigClassAddress()
	{ }

	bool activate(ConfigObject *);
};

extern ConfigClassAddress config_class_address;

#endif /* !CONFIG_CLASS_ADDRESS_H */
