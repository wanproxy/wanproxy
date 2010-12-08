#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event.h>
#include <event/event_callback.h>

#include <io/socket/socket.h>

#include <io/net/udp_client.h>

UDPClient::UDPClient(SocketAddressFamily family)
: log_("/udp/client"),
  family_(family),
  socket_(NULL),
  close_action_(NULL),
  connect_action_(NULL),
  connect_callback_(NULL)
{ }

UDPClient::~UDPClient()
{
	ASSERT(connect_action_ == NULL);
	ASSERT(connect_callback_ == NULL);
	ASSERT(close_action_ == NULL);

	if (socket_ != NULL) {
		delete socket_;
		socket_ = NULL;
	}
}

Action *
UDPClient::connect(const std::string& iface, const std::string& name, EventCallback *ccb)
{
	ASSERT(connect_action_ == NULL);
	ASSERT(connect_callback_ == NULL);
	ASSERT(socket_ == NULL);

	socket_ = Socket::create(family_, SocketTypeDatagram, "udp", name);
	if (socket_ == NULL) {
		ccb->param(Event::Error);
		Action *a = ccb->schedule();

		delete this;

		return (a);
	}

	if (iface != "" && !socket_->bind(iface)) {
		ccb->param(Event::Error);
		Action *a = ccb->schedule();

#if 0
		/*
		 * XXX
		 * Need to schedule close and then delete ourselves.
		 */
		delete this;
#else
		HALT(log_) << "Socket bind failed.";
#endif

		return (a);
	}

	EventCallback *cb = callback(this, &UDPClient::connect_complete);
	connect_action_ = socket_->connect(name, cb);
	connect_callback_ = ccb;

	return (cancellation(this, &UDPClient::connect_cancel));
}

void
UDPClient::connect_cancel(void)
{
	ASSERT(close_action_ == NULL);
	ASSERT(connect_action_ != NULL);

	connect_action_->cancel();
	connect_action_ = NULL;

	if (connect_callback_ != NULL) {
		delete connect_callback_;
		connect_callback_ = NULL;
	} else {
		/* XXX This has a race; caller could cancel after we schedule, but before callback occurs.  */
		/* Caller consumed Socket.  */
		socket_ = NULL;

		delete this;
		return;
	}

	ASSERT(socket_ != NULL);
	EventCallback *cb = callback(this, &UDPClient::close_complete);
	close_action_ = socket_->close(cb);
}

void
UDPClient::connect_complete(Event e)
{
	connect_action_->cancel();
	connect_action_ = NULL;

	e.data_ = (void *)socket_;

	connect_callback_->param(e);
	connect_action_ = connect_callback_->schedule();
	connect_callback_ = NULL;
}

void
UDPClient::close_complete(Event e)
{
	close_action_->cancel();
	close_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		break;
	}

	delete this;
}

Action *
UDPClient::connect(SocketAddressFamily family, const std::string& name, EventCallback *cb)
{
	UDPClient *udp = new UDPClient(family);
	return (udp->connect("", name, cb));
}

Action *
UDPClient::connect(SocketAddressFamily family, const std::string& iface, const std::string& name, EventCallback *cb)
{
	UDPClient *udp = new UDPClient(family);
	return (udp->connect(iface, name, cb));
}
