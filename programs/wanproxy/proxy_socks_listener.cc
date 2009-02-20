#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/channel.h>
#include <io/file_descriptor.h>
#include <io/socket.h>
#include <io/tcp_server.h>

#include "proxy_socks_connection.h"
#include "proxy_socks_listener.h"

ProxySocksListener::ProxySocksListener(const std::string& interface,
				       unsigned local_port)
: log_("/wanproxy/proxy_socks_listener"),
  server_(NULL),
  action_(NULL),
  interface_(interface),
  local_port_(local_port)
{
	server_ = TCPServer::listen(interface, &local_port_);
	if (server_ == NULL) {
		/* XXX
		 * Should retry with a delay in case of a restart?  Or just use
		 * SO_REUSEADDR.  Of course, if it isn't a matter of
		 * SO_REUSEADDR we want to signal an error somehow.  HALT is
		 * probably fine.
		 */
		HALT(log_) << "Unable to create listener.";
	}

	EventCallback *cb = callback(this, &ProxySocksListener::accept_complete);
	action_ = server_->accept(cb);
}

ProxySocksListener::~ProxySocksListener()
{
	NOTREACHED();
}

void
ProxySocksListener::accept_complete(Event e, void *)
{
	action_->cancel();
	action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		return;
	}

	Socket *client = (Socket *)e.data_;
	new ProxySocksConnection(client);

	EventCallback *cb = callback(this, &ProxySocksListener::accept_complete);
	action_ = server_->accept(cb);
}
