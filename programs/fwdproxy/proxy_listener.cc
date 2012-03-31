#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/socket.h>

#include <io/net/tcp_server.h>

#include "proxy_connector.h"
#include "proxy_listener.h"

ProxyListener::ProxyListener(const std::string& name,
			     SocketAddressFamily interface_family,
			     const std::string& interface,
			     SocketAddressFamily remote_family,
			     const std::string& remote_name)
: SimpleServer<TCPServer>("/fwdproxy/proxy/" + name + "/listener", interface_family, interface),
  name_(name),
  remote_family_(remote_family),
  remote_name_(remote_name)
{ }

ProxyListener::~ProxyListener()
{ }

void
ProxyListener::client_connected(Socket *socket)
{
	new ProxyConnector(name_, NULL, socket, remote_family_, remote_name_);
}
