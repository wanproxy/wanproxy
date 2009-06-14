#ifndef	CONFIG_TYPE_LOG_LEVEL_H
#define	CONFIG_TYPE_LOG_LEVEL_H

#include <config/config_type_enum.h>

typedef ConfigTypeEnum<Log::Priority> ConfigTypeLogLevel;

extern ConfigTypeLogLevel config_type_log_level;

#endif /* !CONFIG_TYPE_LOG_LEVEL_H */
