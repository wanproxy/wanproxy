#ifndef	CONFIG_CONFIG_CLASS_LOG_MASK_H
#define	CONFIG_CONFIG_CLASS_LOG_MASK_H

#include <config/config_type_log_level.h>
#include <config/config_type_string.h>

class ConfigClassLogMask : public ConfigClass {
	struct Instance : public ConfigClassInstance {
		std::string regex_;
		Log::Priority mask_;

		bool activate(const ConfigObject *);
	};
public:
	ConfigClassLogMask(void)
	: ConfigClass("log-mask", new ConstructorFactory<ConfigClassInstance, Instance>)
	{
		add_member("regex", &config_type_string, &Instance::regex_);
		add_member("mask", &config_type_log_level, &Instance::mask_);
	}

	~ConfigClassLogMask()
	{ }
};

extern ConfigClassLogMask config_class_log_mask;

#endif /* !CONFIG_CONFIG_CLASS_LOG_MASK_H */
