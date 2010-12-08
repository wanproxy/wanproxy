#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/socket.h>

#include <io/net/tcp_server.h>

#include "proxy_client.h"
#include "proxy_listener.h"

#include "wanproxy_codec_pipe_pair.h"

ProxyListener::ProxyListener(WANProxyCodec *interface_codec,
			     WANProxyCodec *remote_codec,
			     SocketAddressFamily interface_family,
			     const std::string& interface,
			     SocketAddressFamily remote_family,
			     const std::string& remote_name)
: log_("/wanproxy/proxy_listener"),
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
		PipePair *pipe_pair = new WANProxyCodecPipePair(interface_codec_, remote_codec_);
		new ProxyClient(pipe_pair, client, remote_family_, remote_name_);
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
