#ifndef	PROXY_SOCKS_LISTENER_H
#define	PROXY_SOCKS_LISTENER_H

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

#endif /* !PROXY_SOCKS_LISTENER_H */
