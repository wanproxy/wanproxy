#ifndef	PROGRAMS_WANPROXY_SSH_PROXY_LISTENER_H
#define	PROGRAMS_WANPROXY_SSH_PROXY_LISTENER_H

#include <io/socket/simple_server.h>

class Socket;
class TCPServer;
struct WANProxyCodec;

class SSHProxyListener : public SimpleServer<TCPServer> {
	std::string name_;
	WANProxyCodec *interface_codec_;
	WANProxyCodec *remote_codec_;
	SocketAddressFamily remote_family_;
	std::string remote_name_;
public:
	SSHProxyListener(const std::string&, WANProxyCodec *, WANProxyCodec *, SocketAddressFamily,
			 const std::string&, SocketAddressFamily,
			 const std::string&);
	~SSHProxyListener();

private:
	void client_connected(Socket *);
};

#endif /* !PROGRAMS_WANPROXY_SSH_PROXY_LISTENER_H */
