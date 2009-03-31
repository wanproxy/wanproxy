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

ProxyListener::ProxyListener(FlowTable *flow_table, XCodec *local_codec,
			     XCodec *remote_codec, const std::string& interface,
			     unsigned local_port,
			     const std::string& remote_name,
			     unsigned remote_port)
: log_("/wanproxy/proxy_listener"),
  flow_table_(flow_table),
  server_(NULL),
  accept_action_(NULL),
  close_action_(NULL),
  stop_action_(NULL),
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
	accept_action_ = server_->accept(cb);

	Callback *scb = callback(this, &ProxyListener::stop);
	stop_action_ = EventSystem::instance()->register_interest(EventInterestStop, scb);
}

ProxyListener::~ProxyListener()
{
	ASSERT(server_ == NULL);
	ASSERT(accept_action_ == NULL);
	ASSERT(close_action_ == NULL);
	ASSERT(stop_action_ == NULL);
}

void
ProxyListener::accept_complete(Event e)
{
	accept_action_->cancel();
	accept_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::Error:
		INFO(log_) << "Accept failed: " << e;
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		return;
	}

	if (e.type_ == Event::Done) {
		Socket *client = (Socket *)e.data_;
		new ProxyClient(flow_table_, local_codec_, remote_codec_,
				client, remote_name_, remote_port_);
	}

	EventCallback *cb = callback(this, &ProxyListener::accept_complete);
	accept_action_ = server_->accept(cb);
}

void
ProxyListener::close_complete(Event e)
{
	close_action_->cancel();
	close_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		HALT(log_) << "Unexpected event: " << e;
		return;
	}

	delete server_;
	server_ = NULL;

	delete this;
}

void
ProxyListener::stop(void)
{
	stop_action_->cancel();
	stop_action_ = NULL;

	accept_action_->cancel();
	accept_action_ = NULL;

	ASSERT(close_action_ == NULL);

	EventCallback *cb = callback(this, &ProxyListener::close_complete);
	close_action_ = server_->close(cb);
}
