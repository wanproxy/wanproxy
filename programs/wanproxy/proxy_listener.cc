#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/channel.h>
#include <io/file_descriptor.h>
#include <io/socket.h>
#include <io/tcp_server.h>

#include "proxy_client.h"
#include "proxy_listener.h"

ProxyListener::ProxyListener(XCodec *local_codec, XCodec *remote_codec,
			     const std::string& interface, unsigned local_port,
			     const std::string& remote_name,
			     unsigned remote_port)
: log_("/wanproxy/proxy_listener"),
  server_(NULL),
  action_(NULL),
  local_codec_(local_codec),
  remote_codec_(remote_codec),
  interface_(interface),
  local_port_(local_port),
  remote_name_(remote_name),
  remote_port_(remote_port)
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

	EventCallback *cb = callback(this, &ProxyListener::accept_complete);
	action_ = server_->accept(cb);
}

ProxyListener::~ProxyListener()
{
	NOTREACHED();
}

void
ProxyListener::accept_complete(Event e, void *)
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
	new ProxyClient(local_codec_, remote_codec_, client, remote_name_,
			remote_port_);

	EventCallback *cb = callback(this, &ProxyListener::accept_complete);
	action_ = server_->accept(cb);
}
