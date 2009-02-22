#ifndef	WANPROXY_CONFIG_H
#define	WANPROXY_CONFIG_H

#include <set>

class Action;
class FileDescriptor;
class ProxyListener;
class ProxySocksListener;
class XCodec;

class WANProxyConfig {
	LogHandle log_;
	XCodec *codec_;
	std::set<ProxyListener *> listeners_;
	std::set<ProxySocksListener *> socks_listeners_;
	FileDescriptor *config_file_;
	Action *close_action_;
	Action *read_action_;
	Buffer read_buffer_;

public:
	WANProxyConfig(void);
	~WANProxyConfig();

private:
	void close_complete(Event, void *);
	void read_complete(Event, void *);

	void schedule_close(void);
	void schedule_read(void);

	void parse(void);
	void parse(std::vector<std::string>);

	void parse_log_mask(std::vector<std::string>);
	void parse_proxy(std::vector<std::string>);
	void parse_proxy_socks(std::vector<std::string>);

public:
	bool configure(XCodec *, const std::string&);
};

#endif /* !WANPROXY_CONFIG_H */
