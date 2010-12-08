#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/socket/socket.h>

#include <io/net/udp_server.h>

UDPServer *
UDPServer::listen(SocketAddressFamily family, const std::string& name)
{
	Socket *socket = Socket::create(family, SocketTypeDatagram, "udp", name);
	if (socket == NULL) {
		ERROR("/udp/server") << "Unable to create socket.";
		return (NULL);
	}
	/*
	 * XXX
	 * After this we could leak a socket, sigh.  Need a blocking close, or
	 * a pool to return the socket to.
	 */
	if (!socket->bind(name)) {
		ERROR("/udp/server") << "Socket bind failed, leaking socket.";
		return (NULL);
	}
	UDPServer *server = new UDPServer(socket);
	return (server);
}
