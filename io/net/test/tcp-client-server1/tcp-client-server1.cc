/*
 * Copyright (c) 2008-2011 Juli Mallett. All rights reserved.
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

#include <common/test.h>

#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/net/tcp_client.h>
#include <io/net/tcp_server.h>

#include <io/socket/socket.h>

static uint8_t data[65536];

class Connector {
	LogHandle log_;
	TestGroup group_;
	Socket *socket_;
	Action *action_;
	Test *test_;
public:
	Connector(const std::string& suffix, SocketAddressFamily family, const std::string& remote)
	: log_("/connector"),
	  group_("/test/net/socket/connector" + suffix, "Socket connector"),
	  socket_(NULL),
	  action_(NULL)
	{
		test_ = new Test(group_, "TCPClient::connect");
		SocketEventCallback *cb =
			callback(this, &Connector::connect_complete);
		action_ = TCPClient::connect(SocketImplOS, family, remote, cb);
	}

	~Connector()
	{
		{
			Test _(group_, "No outstanding test");
			if (test_ == NULL)
				_.pass();
			else {
				delete test_;
				test_ = NULL;
			}
		}
		{
			Test _(group_, "No pending action");
			if (action_ == NULL)
				_.pass();
			else {
				action_->cancel();
				action_ = NULL;
			}
		}
		{
			Test _(group_, "Socket is closed");
			if (socket_ == NULL)
				_.pass();
			else {
				delete socket_;
				socket_ = NULL;
			}
		}
	}

	void close_complete(void)
	{
		action_->cancel();
		action_ = NULL;

		ASSERT(log_, socket_ != NULL);
		delete socket_;
		socket_ = NULL;
	}

	void connect_complete(Event e, Socket *socket)
	{
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			test_->pass();
			break;
		default:
			ERROR(log_) << "Unexpected event: " << e;
			delete test_;
			test_ = NULL;
			return;
		}
		delete test_;
		test_ = NULL;

		Test _(group_, "TCPClient::connect set Socket pointer.");
		socket_ = socket;
		if (socket_ != NULL)
			_.pass();

		EventCallback *cb = callback(this, &Connector::write_complete);
		Buffer buf(data, sizeof data);
		action_ = socket_->write(&buf, cb);
	}

	void write_complete(Event e)
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

		SimpleCallback *cb = callback(this, &Connector::close_complete);
		action_ = socket_->close(cb);
	}
};

class Listener {
	LogHandle log_;
	TestGroup group_;
	TCPServer *server_;
	Action *action_;
	Connector *connector_;
	Socket *client_;
	Buffer read_buffer_;
public:
	Listener(const std::string& suffix, SocketAddressFamily family, const std::string& name)
	: log_("/listener"),
	  group_("/test/net/tcp_server/listener" + suffix, "Socket listener"),
	  action_(NULL),
	  connector_(NULL),
	  client_(NULL),
	  read_buffer_()
	{
		{
			Test _(group_, "TCPServer::listen");
			server_ = TCPServer::listen(SocketImplOS, family, name);
			if (server_ == NULL)
				return;
			_.pass();
		}

		connector_ = new Connector(suffix, family, server_->getsockname());

		SocketEventCallback *cb = callback(this, &Listener::accept_complete);
		action_ = server_->accept(cb);
	}

	~Listener()
	{
		{
			Test _(group_, "No outstanding client");
			if (client_ == NULL)
				_.pass();
			else {
				delete client_;
				client_ = NULL;
			}
		}
		{
			Test _(group_, "No pending action");
			if (action_ == NULL)
				_.pass();
			else {
				action_->cancel();
				action_ = NULL;
			}
		}
		{
			Test _(group_, "Server not open");
			if (server_ == NULL)
				_.pass();
			else {
				delete server_;
				server_ = NULL;
			}
		}
	}

	void accept_complete(Event e, Socket *socket)
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

		{
			Test _(group_, "Non-NULL client socket");
			client_ = socket;
			if (client_ != NULL)
				_.pass();
		}

		SimpleCallback *cb = callback(this, &Listener::close_complete);
		action_ = server_->close(cb);
	}

	void close_complete(void)
	{
		action_->cancel();
		action_ = NULL;

		ASSERT(log_, server_ != NULL);
		delete server_;
		server_ = NULL;

		EventCallback *cb = callback(this, &Listener::client_read);
		action_ = client_->read(0, cb);
	}

	void client_read(Event e)
	{
		action_->cancel();
		action_ = NULL;

		{
			Test _(group_, "Socket read success");
			switch (e.type_) {
			case Event::Done: {
				read_buffer_.append(e.buffer_);
				_.pass();
				EventCallback *cb =
					callback(this, &Listener::client_read);
				action_ = client_->read(0, cb);
				return;
			}
			case Event::EOS:
				read_buffer_.append(e.buffer_);
				_.pass();
				break;
			default:
				ERROR(log_) << "Unexpected event: " << e;
				return;
			}
		}
		{
			Test _(group_, "Read Buffer length correct");
			if (read_buffer_.length() == sizeof data)
				_.pass();
		}
		{
			Test _(group_, "Read Buffer data correct");
			if (read_buffer_.equal(data, sizeof data))
				_.pass();
		}
		SimpleCallback *cb = callback(this, &Listener::client_close);
		action_ = client_->close(cb);
	}

	void client_close(void)
	{
		action_->cancel();
		action_ = NULL;

		ASSERT(log_, client_ != NULL);
		delete client_;
		client_ = NULL;

		delete connector_;
		connector_ = NULL;
	}
};

int
main(void)
{
	unsigned i;

	for (i = 0; i < sizeof data; i++)
		data[i] = random() % 0xff;

	Listener *l = new Listener("/ip", SocketAddressFamilyIP, "[localhost]:0");
	Listener *l4 = new Listener("/ipv4", SocketAddressFamilyIPv4, "[localhost]:0");
	Listener *l6 = new Listener("/ipv6", SocketAddressFamilyIPv6, "[::1]:0");

	event_main();

	delete l;
	delete l4;
	delete l6;
}
