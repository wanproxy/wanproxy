#ifndef	WANPROXY_CONFIG_H
#define	WANPROXY_CONFIG_H

#include <set>

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
public:
	WANProxyConfig(void);
	~WANProxyConfig();

private:
	void parse(std::deque<std::string>&);

	void parse_flow_monitor(std::deque<std::string>&);
	void parse_flow_table(std::deque<std::string>&);
	void parse_log_mask(std::deque<std::string>&);
	void parse_proxy(std::deque<std::string>&);
	void parse_proxy_socks(std::deque<std::string>&);

public:
	bool configure(XCodec *, const std::string&);
};

#endif /* !WANPROXY_CONFIG_H */
