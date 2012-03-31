#ifndef	PROGRAMS_WANPROXY_PROXY_LISTENER_H
#define	PROGRAMS_WANPROXY_PROXY_LISTENER_H

#include <io/socket/simple_server.h>

class Socket;
class TCPServer;

class ProxyListener : public SimpleServer<TCPServer> {
	std::string name_;
	SocketAddressFamily remote_family_;
	std::string remote_name_;
public:
	ProxyListener(const std::string&, SocketAddressFamily,
		      const std::string&, SocketAddressFamily,
		      const std::string&);
	~ProxyListener();

private:
	void client_connected(Socket *);
};

#endif /* !PROGRAMS_WANPROXY_PROXY_LISTENER_H */
