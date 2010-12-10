#include <event/event_callback.h>

#include <io/socket/socket.h>
#include <io/socket/unix_client.h>

Action *
UnixClient::connect(Socket **socketp, const std::string& name,
		   EventCallback *cb)
{
	Socket *socket = Socket::create(SocketAddressFamilyUnix, SocketTypeStream);
	*socketp = socket;
	if (socket != NULL)
		return (socket->connect(name, cb));
	return (NULL);
}
