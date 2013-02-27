#include <common/buffer.h>

#include <config/config_class.h>
#include <config/config_object.h>

#include <event/action.h>
#include <event/event.h>
#include <event/event_callback.h>
#include <event/event_system.h>

#include <http/http_server.h>

#include <io/socket/socket_types.h>

#include "monitor_client.h"
#include "wanproxy_config_class_interface.h"
#include "wanproxy_config_class_monitor.h"

WANProxyConfigClassMonitor wanproxy_config_class_monitor;

bool
WANProxyConfigClassMonitor::Instance::activate(const ConfigObject *co)
{
	WANProxyConfigClassInterface::Instance *interface =
		dynamic_cast<WANProxyConfigClassInterface::Instance *>(interface_->instance_);
	if (interface == NULL)
		return (false);

	if (interface->host_ == "" || interface->port_ == "")
		return (false);
	std::string interface_address = '[' + interface->host_ + ']' + ':' + interface->port_;
	new HTTPServer<TCPServer, MonitorClient, Config *>(co->config_, interface->family_, interface_address);

	return (true);
}
