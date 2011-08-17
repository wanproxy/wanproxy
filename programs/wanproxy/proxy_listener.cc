#include <common/endian.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/socket.h>

#include <io/net/tcp_server.h>

#include "proxy_connector.h"
#include "proxy_listener.h"

#include "wanproxy_codec_pipe_pair.h"

ProxyListener::ProxyListener(const std::string& name,
			     WANProxyCodec *interface_codec,
			     WANProxyCodec *remote_codec,
			     SocketAddressFamily interface_family,
			     const std::string& interface,
			     SocketAddressFamily remote_family,
			     const std::string& remote_name)
: log_("/wanproxy/proxy/" + name + "/listener"),
  name_(name),
  server_(NULL),
  accept_action_(NULL),
  close_action_(NULL),
  stop_action_(NULL),
  interface_codec_(interface_codec),
  interface_(interface),
  remote_codec_(remote_codec),
  remote_family_(remote_family),
  remote_name_(remote_name)
{
	server_ = TCPServer::listen(interface_family, interface);
	if (server_ == NULL) {
		HALT(log_) << "Unable to create listener.";
	}

	SocketEventCallback *cb = callback(this, &ProxyListener::accept_complete);
	accept_action_ = server_->accept(cb);

	SimpleCallback *scb = callback(this, &ProxyListener::stop);
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
ProxyListener::accept_complete(Event e, Socket *socket)
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
		PipePair *pipe_pair = new WANProxyCodecPipePair(interface_codec_, remote_codec_);
		new ProxyConnector(name_, pipe_pair, socket, remote_family_, remote_name_);
	}

	SocketEventCallback *cb = callback(this, &ProxyListener::accept_complete);
	accept_action_ = server_->accept(cb);
}

void
ProxyListener::close_complete(void)
{
	close_action_->cancel();
	close_action_ = NULL;

	ASSERT(server_ != NULL);
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

	SimpleCallback *cb = callback(this, &ProxyListener::close_complete);
	close_action_ = server_->close(cb);
}
