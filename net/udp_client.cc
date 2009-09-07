#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/socket.h>

#include <net/udp_client.h>

Action *
UDPClient::connect(Socket **socketp, SocketAddressFamily family,
		   const std::string& name, EventCallback *cb)
{
	Socket *socket = Socket::create(family, SocketTypeDatagram, "udp", name);
	*socketp = socket;
	if (socket != NULL)
		return (socket->connect(name, cb));
	return (NULL);
}
