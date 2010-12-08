#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

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
