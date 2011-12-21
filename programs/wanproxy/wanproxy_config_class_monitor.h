#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_MONITOR_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_MONITOR_H

#include <set>

#include <config/config_type_pointer.h>

class WANProxyConfigClassMonitor : public ConfigClass {
	std::set<ConfigObject *> object_listener_set_;
public:
	WANProxyConfigClassMonitor(void)
	: ConfigClass("monitor"),
	  object_listener_set_()
	{
		add_member("interface", &config_type_pointer);
	}

	/* XXX So wrong.  */
	~WANProxyConfigClassMonitor()
	{ }

	bool activate(ConfigObject *);
};

extern WANProxyConfigClassMonitor wanproxy_config_class_monitor;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_MONITOR_H */
