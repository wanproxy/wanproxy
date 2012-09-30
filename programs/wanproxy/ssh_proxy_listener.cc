#include <common/endian.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/socket.h>

#include <io/net/tcp_server.h>

#include "ssh_proxy_connector.h"
#include "ssh_proxy_listener.h"

#include "wanproxy_codec_pipe_pair.h"

SSHProxyListener::SSHProxyListener(const std::string& name,
				   WANProxyCodec *interface_codec,
				   WANProxyCodec *remote_codec,
				   SocketAddressFamily interface_family,
				   const std::string& interface,
				   SocketAddressFamily remote_family,
				   const std::string& remote_name)
: SimpleServer<TCPServer>("/wanproxy/proxy/" + name + "/listener", interface_family, interface),
  name_(name),
  interface_codec_(interface_codec),
  remote_codec_(remote_codec),
  remote_family_(remote_family),
  remote_name_(remote_name)
{ }

SSHProxyListener::~SSHProxyListener()
{ }

void
SSHProxyListener::client_connected(Socket *socket)
{
	PipePair *pipe_pair = new WANProxyCodecPipePair(interface_codec_, remote_codec_);
	new SSHProxyConnector(name_, pipe_pair, socket, remote_family_, remote_name_);
}
