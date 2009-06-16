#include <config/config_class.h>
#include <config/config_object.h>
#include <config/config_value.h>

#include "wanproxy_config_class_proxy_socks.h"

WANProxySocksConfigClassProxySocks wanproxy_config_class_proxy_socks;

bool
WANProxySocksConfigClassProxySocks::activate(ConfigObject *)
{
	/* Extract interface.  */

	return (true);
}
