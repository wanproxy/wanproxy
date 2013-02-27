#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_MONITOR_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_MONITOR_H

#include <config/config_type_pointer.h>

class MonitorListener;

class WANProxyConfigClassMonitor : public ConfigClass {
	struct Instance : public ConfigClassInstance {
		ConfigObject *interface_;

		Instance(void)
		: interface_(NULL)
		{ }

		bool activate(const ConfigObject *);
	};
public:
	WANProxyConfigClassMonitor(void)
	: ConfigClass("monitor", new ConstructorFactory<ConfigClassInstance, Instance>)
	{
		add_member("interface", &config_type_pointer, &Instance::interface_);
	}

	/* XXX So wrong.  */
	~WANProxyConfigClassMonitor()
	{ }
};

extern WANProxyConfigClassMonitor wanproxy_config_class_monitor;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_MONITOR_H */
