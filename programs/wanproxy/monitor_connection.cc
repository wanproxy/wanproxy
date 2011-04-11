#include <common/endian.h>

#include <event/event_callback.h>

#include <io/socket/socket.h>

#include "proxy_connector.h"
#include "monitor_connection.h"

MonitorConnection::MonitorConnection(const std::string& name, Socket *client)
: log_("/wanproxy/monitor/" + name + "/connection"),
  name_(name),
  client_(client),
  action_(NULL)
{
	/* XXX Write config?  Schedule timer?  */
	schedule_close();
}

MonitorConnection::~MonitorConnection()
{
	ASSERT(action_ == NULL);
}

void
MonitorConnection::write_complete(Event e)
{
	action_->cancel();
	action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
	}

	schedule_close();
}

void
MonitorConnection::close_complete(void)
{
	action_->cancel();
	action_ = NULL;

	ASSERT(client_ != NULL);
	delete client_;
	client_ = NULL;

	delete this;
}

void
MonitorConnection::schedule_close(void)
{
	ASSERT(action_ == NULL);

	ASSERT(client_ != NULL);
	Callback *cb = callback(this, &MonitorConnection::close_complete);
	action_ = client_->close(cb);
}
