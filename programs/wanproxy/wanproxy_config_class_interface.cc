#include <config/config_class.h>
#include <config/config_object.h>
#include <config/config_value.h>

#include "wanproxy_config_class_interface.h"

WANProxyConfigClassInterface wanproxy_config_class_interface;

bool
WANProxyConfigClassInterface::activate(ConfigObject *)
{
	ERROR("/wanproxy/config/interface") << __PRETTY_FUNCTION__ << " not yet implemented.";
	/* XXX Stubbed out for testing.  */
	return (true);
}
