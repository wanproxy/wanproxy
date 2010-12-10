#include <event/event_callback.h>

#include <io/socket/socket.h>
#include <io/socket/unix_server.h>

UnixServer *
UnixServer::listen(const std::string& name)
{
	Socket *socket = Socket::create(SocketAddressFamilyUnix, SocketTypeStream);
	if (socket == NULL) {
		ERROR("/unix/server") << "Unable to create socket.";
		return (NULL);
	}
	/*
	 * XXX
	 * After this we could leak a socket, sigh.  Need a blocking close, or
	 * a pool to return the socket to.
	 */
	if (!socket->bind(name)) {
		ERROR("/unix/server") << "Socket bind failed, leaking socket.";
		return (NULL);
	}
	if (!socket->listen(10)) {
		ERROR("/unix/server") << "Socket listen failed, leaking socket.";
		return (NULL);
	}
	UnixServer *server = new UnixServer(socket);
	return (server);
}
