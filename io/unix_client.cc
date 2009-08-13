#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/socket.h>
#include <io/unix_client.h>

Action *
UnixClient::connect(Socket **socketp, const std::string& name,
		   EventCallback *cb)
{
	Socket *socket = Socket::create(SocketAddressFamilyUnix, SocketTypeStream, "");
	ASSERT(socket != NULL);
	*socketp = socket;
	return (socket->connect(name, cb));
}
