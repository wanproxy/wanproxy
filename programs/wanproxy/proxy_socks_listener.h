#ifndef	PROGRAMS_WANPROXY_PROXY_SOCKS_LISTENER_H
#define	PROGRAMS_WANPROXY_PROXY_SOCKS_LISTENER_H

#include <io/socket/simple_server.h>

class Socket;
class TCPServer;
class XCodec;

class ProxySocksListener : public SimpleServer<TCPServer> {
	std::string name_;

public:
	ProxySocksListener(const std::string&, SocketAddressFamily, const std::string&);
	~ProxySocksListener();

private:
	void client_connected(Socket *);
};

#endif /* !PROGRAMS_WANPROXY_PROXY_SOCKS_LISTENER_H */
