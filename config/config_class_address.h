#ifndef	CONFIG_CONFIG_CLASS_ADDRESS_H
#define	CONFIG_CONFIG_CLASS_ADDRESS_H

#include <config/config_type_address_family.h>
#include <config/config_type_string.h>

class ConfigClassAddress : public ConfigClass {
public:
	struct Instance : public ConfigClassInstance {
		SocketAddressFamily family_;
		std::string host_;
		std::string port_;
		std::string path_;

		Instance(void)
		: family_(SocketAddressFamilyUnspecified),
		  host_(""),
		  port_(""),
		  path_("")
		{
		}

		bool activate(const ConfigObject *);
	};

	ConfigClassAddress(const std::string& xname = "address")
	: ConfigClass(xname, new ConstructorFactory<ConfigClassInstance, Instance>)
	{
		add_member("family", &config_type_address_family, &Instance::family_);
		add_member("host", &config_type_string, &Instance::host_);
		add_member("port", &config_type_string, &Instance::port_); /* XXX enum?  */
		add_member("path", &config_type_string, &Instance::path_);
	}

	~ConfigClassAddress()
	{ }
};

extern ConfigClassAddress config_class_address;

#endif /* !CONFIG_CONFIG_CLASS_ADDRESS_H */
