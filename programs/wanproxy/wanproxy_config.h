#ifndef	WANPROXY_CONFIG_H
#define	WANPROXY_CONFIG_H

#include <set>

class Action;
class FileDescriptor;
class FlowMonitor;
class FlowTable;
class ProxyListener;
class ProxySocksListener;
class XCodec;

class WANProxyConfig {
	LogHandle log_;
	XCodec *codec_;
	FlowMonitor *flow_monitor_;
	std::map<std::string, FlowTable *> flow_tables_;
	std::set<ProxyListener *> proxy_listeners_;
	std::set<ProxySocksListener *> proxy_socks_listeners_;
	FileDescriptor *config_file_;
	Action *close_action_;
	Action *read_action_;
	Buffer read_buffer_;

public:
	WANProxyConfig(void);
	~WANProxyConfig();

private:
	void close_complete(Event);
	void read_complete(Event);

	void schedule_close(void);
	void schedule_read(void);

	void parse(void);
	void parse(std::deque<std::string>);

	void parse_flow_monitor(std::deque<std::string>);
	void parse_flow_table(std::deque<std::string>);
	void parse_log_mask(std::deque<std::string>);
	void parse_proxy(std::deque<std::string>);
	void parse_proxy_socks(std::deque<std::string>);

public:
	bool configure(XCodec *, const std::string&);
};

#endif /* !WANPROXY_CONFIG_H */
