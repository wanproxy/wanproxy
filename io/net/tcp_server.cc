#include <event/event_callback.h>

#include <io/socket/socket.h>

#include <io/net/tcp_server.h>

TCPServer *
TCPServer::listen(SocketAddressFamily family, const std::string& name)
{
	Socket *socket = Socket::create(family, SocketTypeStream, "tcp", name);
	if (socket == NULL) {
		ERROR("/tcp/server") << "Unable to create socket.";
		return (NULL);
	}
	/*
	 * XXX
	 * After this we could leak a socket, sigh.  Need a blocking close, or
	 * a pool to return the socket to.
	 */
	if (!socket->bind(name)) {
		ERROR("/tcp/server") << "Socket bind failed, leaking socket.";
		return (NULL);
	}
	if (!socket->listen()) {
		ERROR("/tcp/server") << "Socket listen failed, leaking socket.";
		return (NULL);
	}
	TCPServer *server = new TCPServer(socket);
	return (server);
}
