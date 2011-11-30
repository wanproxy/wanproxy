#include <common/endian.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/socket.h>

#include <io/net/tcp_server.h>

#include "proxy_socks_connection.h"
#include "proxy_socks_listener.h"

ProxySocksListener::ProxySocksListener(const std::string& name, SocketAddressFamily family, const std::string& interface)
: SimpleServer<TCPServer>("/wanproxy/proxy/" + name + "/socks/listener", family, interface),
  name_(name)
{ }

ProxySocksListener::~ProxySocksListener()
{ }

void
ProxySocksListener::client_connected(Socket *socket)
{
	new ProxySocksConnection(name_, socket);
}
