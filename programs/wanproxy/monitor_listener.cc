#include <common/endian.h>

#include <event/event_callback.h>
#include <event/event_system.h>

#include <io/socket/socket.h>

#include <io/net/tcp_server.h>

#include "monitor_connection.h"
#include "monitor_listener.h"

MonitorListener::MonitorListener(const std::string& name, SocketAddressFamily family, const std::string& interface)
: log_("/wanproxy/monitor/" + name + "/listener"),
  name_(name),
  server_(NULL),
  accept_action_(NULL),
  close_action_(NULL),
  stop_action_(NULL),
  interface_(interface)
{
	server_ = TCPServer::listen(family, interface);
	if (server_ == NULL) {
		HALT(log_) << "Unable to create listener.";
	}

	EventCallback *cb = callback(this, &MonitorListener::accept_complete);
	accept_action_ = server_->accept(cb);

	Callback *scb = callback(this, &MonitorListener::stop);
	stop_action_ = EventSystem::instance()->register_interest(EventInterestStop, scb);
}

MonitorListener::~MonitorListener()
{
	ASSERT(server_ == NULL);
	ASSERT(accept_action_ == NULL);
	ASSERT(close_action_ == NULL);
	ASSERT(stop_action_ == NULL);
}

void
MonitorListener::accept_complete(Event e)
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
		new MonitorConnection(name_, client);
	}

	EventCallback *cb = callback(this, &MonitorListener::accept_complete);
	accept_action_ = server_->accept(cb);
}

void
MonitorListener::close_complete(void)
{
	close_action_->cancel();
	close_action_ = NULL;

	ASSERT(server_ != NULL);
	delete server_;
	server_ = NULL;

	delete this;
}

void
MonitorListener::stop(void)
{
	stop_action_->cancel();
	stop_action_ = NULL;

	accept_action_->cancel();
	accept_action_ = NULL;

	ASSERT(close_action_ == NULL);

	Callback *cb = callback(this, &MonitorListener::close_complete);
	close_action_ = server_->close(cb);
}
