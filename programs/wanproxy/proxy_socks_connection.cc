/*
 * Copyright (c) 2009-2013 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <common/endian.h>

#include <event/event_callback.h>

#include <io/socket/socket.h>

#include "proxy_connector.h"
#include "proxy_socks_connection.h"

ProxySocksConnection::ProxySocksConnection(const std::string& name, Socket *client)
: log_("/wanproxy/proxy/" + name + "/socks/connection"),
  name_(name),
  client_(client),
  action_(NULL),
  state_(GetSOCKSVersion),
  network_port_(0),
  network_address_(),
  socks5_authenticated_(false),
  socks5_remote_name_()
{
	schedule_read(1);
}

ProxySocksConnection::~ProxySocksConnection()
{
	ASSERT(log_, action_ == NULL);
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
		e.buffer_.extract(&network_port_);
		network_port_ = BigEndian::decode(network_port_);

		state_ = GetSOCKS4Address;
		schedule_read(4);
		break;
	case GetSOCKS4Address:
		ASSERT(log_, e.buffer_.length() == 4);
		network_address_ = e.buffer_;

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
		case 0x04:
			state_ = GetSOCKS5Address6;
			schedule_read(16);
			break;
		default:
			schedule_close();
			break;
		}
		break;
	case GetSOCKS5Address:
		ASSERT(log_, e.buffer_.length() == 4);
		network_address_ = e.buffer_;

		state_ = GetSOCKS5Port;
		schedule_read(2);
		break;
	case GetSOCKS5Address6:
		ASSERT(log_, e.buffer_.length() == 16);
		network_address_ = e.buffer_;

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
		e.buffer_.extract(&network_port_);
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
		ASSERT(log_, !socks5_authenticated_);
		ASSERT(log_, socks5_remote_name_ == "");
		ASSERT(log_, network_address_.empty());
		ASSERT(log_, network_port_ == 0);

		socks5_authenticated_ = true;
		state_ = GetSOCKSVersion;
		schedule_read(1);
		return;
	}

	std::ostringstream remote_name;

	SocketAddressFamily family;
	if (state_ == GetSOCKS5Port && socks5_remote_name_ != "") {
		ASSERT(log_, socks5_authenticated_);
		ASSERT(log_, network_address_.empty());

		remote_name << '[' << socks5_remote_name_ << ']' << ':' << network_port_;

		family = SocketAddressFamilyIP;
	} else {
		remote_name << '[';
		switch (network_address_.length()) {
		case 4:
			remote_name << (unsigned)network_address_.peek() << '.';
			network_address_.skip(1);
			remote_name << (unsigned)network_address_.peek() << '.';
			network_address_.skip(1);
			remote_name << (unsigned)network_address_.peek() << '.';
			network_address_.skip(1);
			remote_name << (unsigned)network_address_.peek();
			network_address_.skip(1);
			family = SocketAddressFamilyIPv4;
			break;
		case 16:
			for (;;) {
				uint8_t bytes[2];
				char hex[5];

				network_address_.moveout(bytes, sizeof bytes);

				if (bytes[0] == 0 && bytes[1] == 0)
					strlcpy(hex, "0", sizeof hex);
				else
					snprintf(hex, sizeof hex, "%0x%02x", bytes[0], bytes[1]);
				remote_name << hex;

				if (network_address_.empty())
					break;
				remote_name << ":";
			}
			family = SocketAddressFamilyIPv6;
			break;
		default:
			NOTREACHED(log_);
		}
		remote_name << ']';
		remote_name << ':' << network_port_;
	}

	new ProxyConnector(name_, NULL, client_, SocketImplOS, family, remote_name.str());

	client_ = NULL;
	delete this;
}

void
ProxySocksConnection::close_complete(void)
{
	action_->cancel();
	action_ = NULL;

	ASSERT(log_, client_ != NULL);
	delete client_;
	client_ = NULL;

	delete this;
	return;
}

void
ProxySocksConnection::schedule_read(size_t amount)
{
	ASSERT(log_, action_ == NULL);

	ASSERT(log_, client_ != NULL);
	EventCallback *cb = callback(this, &ProxySocksConnection::read_complete);
	action_ = client_->read(amount, cb);
}

void
ProxySocksConnection::schedule_write(void)
{
	ASSERT(log_, action_ == NULL);

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
		if (socks5_remote_name_ != "" || network_address_.empty()) {
			response.append((uint8_t)0x03);
			response.append((uint8_t)socks5_remote_name_.length());
			response.append(socks5_remote_name_);
		} else {
			uint32_t address;
			network_address_.extract(&address);
			address = BigEndian::encode(address);

			response.append((uint8_t)0x01);
			response.append(&address);
		}

		uint16_t port = BigEndian::encode(network_port_);
		response.append(&port);
		break;
	}
	default:
		NOTREACHED(log_);
	}

	ASSERT(log_, client_ != NULL);
	EventCallback *cb = callback(this, &ProxySocksConnection::write_complete);
	action_ = client_->write(&response, cb);
}

void
ProxySocksConnection::schedule_close(void)
{
	ASSERT(log_, action_ == NULL);

	ASSERT(log_, client_ != NULL);
	SimpleCallback *cb = callback(this, &ProxySocksConnection::close_complete);
	action_ = client_->close(cb);
}
