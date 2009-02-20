#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/channel.h>
#include <io/file_descriptor.h>
#include <io/socket.h>

#include "proxy_client.h"
#include "proxy_socks_connection.h"

ProxySocksConnection::ProxySocksConnection(Channel *client)
: log_("/wanproxy/proxy_socks_connection"),
  client_(client),
  action_(NULL),
  state_(GetVersion),
  network_port_(0),
  network_address_(0)
{
	schedule_read(1);
}

ProxySocksConnection::~ProxySocksConnection()
{
	ASSERT(action_ == NULL);
}

void
ProxySocksConnection::read_complete(Event e, void *)
{
	action_->cancel();
	action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		schedule_close();
		return;
	}

	switch (state_) {
	case GetVersion:
		if (e.buffer_.peek() != 0x04) {
			schedule_close();
			break;
		}
		state_ = GetCommand;
		schedule_read(1);
		break;
	case GetCommand:
		if (e.buffer_.peek() != 0x01) {
			schedule_close();
			break;
		}
		state_ = GetPort;
		schedule_read(2);
		break;
	case GetPort:
		e.buffer_.copyout((uint8_t *)&network_port_, 2);
		network_port_ = BigEndian::decode(network_port_);

		state_ = GetAddress;
		schedule_read(4);
		break;
	case GetAddress:
		e.buffer_.copyout((uint8_t *)&network_address_, 4);
		network_address_ = BigEndian::decode(network_address_);

		state_ = GetUser;
		schedule_read(1);
		break;
	case GetUser:
		if (e.buffer_.peek() == 0x00) {
			schedule_write();
			break;
		}
		schedule_read(1);
		break;
	}
}

void
ProxySocksConnection::write_complete(Event e, void *)
{
	action_->cancel();
	action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		schedule_close();
		return;
	}

	new ProxyClient(NULL, NULL, client_, network_address_,
			network_port_);
	client_ = NULL;
	delete this;
}

void
ProxySocksConnection::close_complete(Event e, void *)
{
	action_->cancel();
	action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		/* XXX Never sure what to do here.  */
		ERROR(log_) << "Unexpected event: " << e;
		schedule_close();
		return;
	}

	delete client_;
	client_ = NULL;

	delete this;
	return;
}

void
ProxySocksConnection::schedule_read(size_t amount)
{
	ASSERT(action_ == NULL);

	ASSERT(client_ != NULL);
	EventCallback *cb = callback(this, &ProxySocksConnection::read_complete);
	action_ = client_->read(cb, amount);
}

void
ProxySocksConnection::schedule_write(void)
{
	ASSERT(action_ == NULL);

	static const uint8_t response[] = {
		0x00,
		0x5a,
		0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};

	INFO(log_) << "Accepted SOCKS connection request.";

	Buffer buf(response, sizeof response);

	ASSERT(client_ != NULL);
	EventCallback *cb = callback(this, &ProxySocksConnection::write_complete);
	action_ = client_->write(&buf, cb);
}

void
ProxySocksConnection::schedule_close(void)
{
	ASSERT(action_ == NULL);

	ASSERT(client_ != NULL);
	EventCallback *cb = callback(this, &ProxySocksConnection::close_complete);
	action_ = client_->close(cb);
}
