#include <sys/socket.h>
#include <netinet/in.h>

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/file_descriptor.h>
#include <io/socket.h>
#include <io/tcp_client.h>

Action *
TCPClient::connect(TCPClient **clientp, const std::string& name, int port,
		   EventCallback *cb)
{
	Socket *socket = Socket::create(PF_INET, SOCK_STREAM, "tcp");
	if (socket == NULL)
		return (NULL);
	TCPClient *client = new TCPClient(socket);
	*clientp = client;
	return (socket->connect(name, port, cb));
}
