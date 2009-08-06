#ifndef	CONFIG_CLASS_LOG_MASK_H
#define	CONFIG_CLASS_LOG_MASK_H

#include <config/config_type_log_level.h>
#include <config/config_type_string.h>

class ConfigObject;

class ConfigClassLogMask : public ConfigClass {
public:
	ConfigClassLogMask(void)
	: ConfigClass("log-mask")
	{
		add_member("regex", &config_type_string);
		add_member("mask", &config_type_log_level);
	}

	~ConfigClassLogMask()
	{ }

	bool activate(ConfigObject *);
};

extern ConfigClassLogMask config_class_log_mask;

#endif /* !CONFIG_CLASS_LOG_MASK_H */
