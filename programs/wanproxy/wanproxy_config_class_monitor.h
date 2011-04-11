#ifndef	WANPROXY_CONFIG_CLASS_MONITOR_H
#define	WANPROXY_CONFIG_CLASS_MONITOR_H

#include <config/config_type_pointer.h>

class MonitorListener;

class WANProxyConfigClassMonitor : public ConfigClass {
	std::map<ConfigObject *, MonitorListener *> object_listener_map_;
public:
	WANProxyConfigClassMonitor(void)
	: ConfigClass("monitor"),
	  object_listener_map_()
	{
		add_member("interface", &config_type_pointer);
	}

	/* XXX So wrong.  */
	~WANProxyConfigClassMonitor()
	{ }

	bool activate(ConfigObject *);
};

extern WANProxyConfigClassMonitor wanproxy_config_class_monitor;

#endif /* !WANPROXY_CONFIG_CLASS_MONITOR_H */
