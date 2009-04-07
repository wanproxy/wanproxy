#include <sys/socket.h>
#include <netinet/in.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/file_descriptor.h>
#include <io/socket.h>

#include <net/tcp_server.h>

TCPServer *
TCPServer::listen(const std::string& name, unsigned *portp)
{
	Socket *socket = Socket::create(PF_INET, SOCK_STREAM, "tcp");
	if (socket == NULL) {
		ERROR("/tcp/server") << "Unable to create socket.";
		return (NULL);
	}
	/*
	 * XXX
	 * After this we could leak a socket, sigh.  Need a blocking close, or
	 * a pool to return the socket to.
	 */
	if (!socket->bind(name, portp)) {
		ERROR("/tcp/server") << "Socket bind failed, leaking socket.";
		return (NULL);
	}
	if (!socket->listen(10)) {
		ERROR("/tcp/server") << "Socket listen failed, leaking socket.";
		return (NULL);
	}
	TCPServer *server = new TCPServer(socket);
	return (server);
}
