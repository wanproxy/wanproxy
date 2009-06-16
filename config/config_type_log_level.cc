#include <config/config_type_log_level.h>

struct ConfigTypeLogLevel::Mapping config_type_log_level_map[] = {
	{ "EMERG",	Log::Emergency },
	{ "ALERT",	Log::Alert },
	{ "CRIT",	Log::Critical },
	{ "ERR",	Log::Error },
	{ "WARNING",	Log::Warning },
	{ "NOTICE",	Log::Notice },
	{ "INFO",	Log::Info },
	{ "DEBUG",	Log::Debug },
	{ NULL,		Log::Debug }
};

ConfigTypeLogLevel
	config_type_log_level("log-level", config_type_log_level_map);
