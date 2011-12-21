#ifndef	PROGRAMS_WANPROXY_MONITOR_CLIENT_H
#define	PROGRAMS_WANPROXY_MONITOR_CLIENT_H

#include <http/http_server.h>

class Config;

class MonitorClient : public HTTPServerHandler {
	Config *config_;
public:
	MonitorClient(Config *config, Socket *client)
	: HTTPServerHandler(client),
	  config_(config)
	{ }

	~MonitorClient()
	{ }

private:
	void handle_request(const std::string&, const std::string&, HTTPProtocol::Request);
};

#endif /* !PROGRAMS_WANPROXY_MONITOR_CLIENT_H */
