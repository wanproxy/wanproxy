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

ProxySocksConnection::ProxySocksConnection(Socket *client)
: log_("/wanproxy/proxy_socks_connection"),
  client_(client),
  action_(NULL),
  state_(GetSOCKSVersion),
  network_port_(0),
  network_address_(0),
  socks5_authenticated_(false),
  socks5_remote_name_()
{
	schedule_read(1);
}

ProxySocksConnection::~ProxySocksConnection()
{
	ASSERT(action_ == NULL);
}

void
ProxySocksConnection::read_complete(Event e)
{
	action_->cancel();
	action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	case Event::EOS:
		INFO(log_) << "Client closed before connection established.";
		schedule_close();
		return;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		schedule_close();
		return;
	}

	switch (state_) {
	case GetSOCKSVersion:
		switch (e.buffer_.peek()) {
		case 0x04:
			state_ = GetSOCKS4Command;
			schedule_read(1);
			break;
		case 0x05:
			if (socks5_authenticated_) {
				state_ = GetSOCKS5Command;
				schedule_read(1);
			} else {
				state_ = GetSOCKS5AuthLength;
				schedule_read(1);
			}
			break;
		default:
			schedule_close();
			break;
		}
		break;

		/* SOCKS4 */
	case GetSOCKS4Command:
		if (e.buffer_.peek() != 0x01) {
			schedule_close();
			break;
		}
		state_ = GetSOCKS4Port;
		schedule_read(2);
		break;
	case GetSOCKS4Port:
		e.buffer_.copyout((uint8_t *)&network_port_, 2);
		network_port_ = BigEndian::decode(network_port_);

		state_ = GetSOCKS4Address;
		schedule_read(4);
		break;
	case GetSOCKS4Address:
		e.buffer_.copyout((uint8_t *)&network_address_, 4);
		network_address_ = BigEndian::decode(network_address_);

		state_ = GetSOCKS4User;
		schedule_read(1);
		break;
	case GetSOCKS4User:
		if (e.buffer_.peek() == 0x00) {
			schedule_write();
			break;
		}
		schedule_read(1);
		break;

		/* SOCKS5 */
	case GetSOCKS5AuthLength:
		state_ = GetSOCKS5Auth;
		schedule_read(e.buffer_.peek());
		break;
	case GetSOCKS5Auth:
		while (!e.buffer_.empty()) {
			if (e.buffer_.peek() == 0x00) {
				schedule_write();
				break;
			}
			e.buffer_.skip(1);
		}
		if (e.buffer_.empty()) {
			schedule_close();
			break;
		}
		break;

	case GetSOCKS5Command:
		if (e.buffer_.peek() != 0x01) {
			schedule_close();
			break;
		}
		state_ = GetSOCKS5Reserved;
		schedule_read(1);
		break;
	case GetSOCKS5Reserved:
		if (e.buffer_.peek() != 0x00) {
			schedule_close();
			break;
		}
		state_ = GetSOCKS5AddressType;
		schedule_read(1);
		break;
	case GetSOCKS5AddressType:
		switch (e.buffer_.peek()) {
		case 0x01:
			state_ = GetSOCKS5Address;
			schedule_read(4);
			break;
		case 0x03:
			state_ = GetSOCKS5NameLength;
			schedule_read(1);
			break;
		default:
			schedule_close();
			break;
		}
		break;
	case GetSOCKS5Address:
		e.buffer_.copyout((uint8_t *)&network_address_, 4);
		network_address_ = BigEndian::decode(network_address_);

		state_ = GetSOCKS5Port;
		schedule_read(2);
		break;
	case GetSOCKS5NameLength:
		state_ = GetSOCKS5Name;
		schedule_read(e.buffer_.peek());
		break;
	case GetSOCKS5Name:
		while (!e.buffer_.empty()) {
			socks5_remote_name_ += (char)e.buffer_.peek();
			e.buffer_.skip(1);
		}
		state_ = GetSOCKS5Port;
		schedule_read(2);
		break;
	case GetSOCKS5Port:
		e.buffer_.copyout((uint8_t *)&network_port_, 2);
		network_port_ = BigEndian::decode(network_port_);

		schedule_write();
		break;
	}
}

void
ProxySocksConnection::write_complete(Event e)
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

	if (state_ == GetSOCKS5Auth) {
		ASSERT(!socks5_authenticated_);
		ASSERT(socks5_remote_name_ == "");
		ASSERT(network_address_ == 0);
		ASSERT(network_port_ == 0);

		socks5_authenticated_ = true;
		state_ = GetSOCKSVersion;
		schedule_read(1);
		return;
	}

	if (state_ == GetSOCKS5Port && socks5_remote_name_ != "") {
		ASSERT(socks5_authenticated_);
		ASSERT(network_address_ == 0);

		new ProxyClient(NULL, NULL, client_, socks5_remote_name_,
				network_port_);
	} else {
		new ProxyClient(NULL, NULL, client_, network_address_,
				network_port_);
	}
	client_ = NULL;
	delete this;
}

void
ProxySocksConnection::close_complete(Event e)
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
	action_ = client_->read(amount, cb);
}

void
ProxySocksConnection::schedule_write(void)
{
	ASSERT(action_ == NULL);

	static const uint8_t socks4_connected[] = {
		0x00,
		0x5a,
		0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};

	static const uint8_t socks5_authenticated[] = {
		0x05,
		0x00,
	};

	static const uint8_t socks5_connected[] = {
		0x05,
		0x00,
		0x00,
	};

	Buffer response;
	
	switch (state_) {
	case GetSOCKS4User:
		response.append(socks4_connected, sizeof socks4_connected);
		break;
	case GetSOCKS5Auth:
		response.append(socks5_authenticated, sizeof socks5_authenticated);
		break;
	case GetSOCKS5Port: {
		response.append(socks5_connected, sizeof socks5_connected);

		/* XXX It's fun to lie about what kind of name we were given.  */
		if (socks5_remote_name_ != "" || network_address_ == 0) {
			response.append(0x03);
			response.append(socks5_remote_name_.length());
			response.append(socks5_remote_name_);
		} else {
			uint32_t address = BigEndian::encode(network_address_);

			response.append(0x01);
			response.append((const uint8_t *)&address, sizeof address);
		}

		uint16_t port = BigEndian::encode(network_port_);
		response.append((const uint8_t *)&port, sizeof port);
		break;
	}
	default:
		NOTREACHED();
	}

	ASSERT(client_ != NULL);
	EventCallback *cb = callback(this, &ProxySocksConnection::write_complete);
	action_ = client_->write(&response, cb);
}

void
ProxySocksConnection::schedule_close(void)
{
	ASSERT(action_ == NULL);

	ASSERT(client_ != NULL);
	EventCallback *cb = callback(this, &ProxySocksConnection::close_complete);
	action_ = client_->close(cb);
}
