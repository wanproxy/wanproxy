/*
 * Copyright (c) 2008-2013 Juli Mallett. All rights reserved.
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

#include <common/buffer.h>
#include <common/test.h>

#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/socket/socket.h>

static uint8_t data[65536];

class Connector {
	LogHandle log_;
	TestGroup group_;
	Socket *socket_;
	Action *action_;
	Test *test_;
public:
	Connector(const std::string& remote)
	: log_("/connector"),
	  group_("/test/io/socket/connector", "Socket connector"),
	  socket_(NULL),
	  action_(NULL)
	{
		{
			Test _(group_, "Socket::create");
			socket_ = Socket::create(SocketAddressFamilyIPv4, SocketTypeStream, "tcp");
			if (socket_ == NULL)
				return;
			_.pass();
		}
		test_ = new Test(group_, "Socket::connect");
		EventCallback *cb =
			callback(this, &Connector::connect_complete);
		action_ = socket_->connect(remote, cb);
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

	void connect_complete(Event e)
	{
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			test_->pass();
			break;
		default:
			ERROR(log_) << "Unexpected event: " << e;
			break;
		}
		delete test_;
		test_ = NULL;

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
	Socket *socket_;
	Action *action_;
	Connector *connector_;
	Socket *client_;
	Buffer read_buffer_;
public:
	Listener(void)
	: log_("/listener"),
	  group_("/test/io/socket/listener", "Socket listener"),
	  action_(NULL),
	  connector_(NULL),
	  client_(NULL),
	  read_buffer_()
	{
		{
			Test _(group_, "Socket::create");
			socket_ = Socket::create(SocketAddressFamilyIPv4, SocketTypeStream, "tcp");
			if (socket_ == NULL)
				return;
			_.pass();
		}
		{
			Test _(group_, "Socket bind to localhost, port=0");
			if (socket_->bind("[localhost]:0"))
				_.pass();
		}
		{
			Test _(group_, "Socket listen");
			if (!socket_->listen())
				return;
			_.pass();
		}

		connector_ = new Connector(socket_->getsockname());

		SocketEventCallback *cb = callback(this, &Listener::accept_complete);
		action_ = socket_->accept(cb);
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
			Test _(group_, "Socket not open");
			if (socket_ == NULL)
				_.pass();
			else {
				delete socket_;
				socket_ = NULL;
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
		action_ = socket_->close(cb);
	}

	void close_complete(void)
	{
		action_->cancel();
		action_ = NULL;

		ASSERT(log_, socket_ != NULL);
		delete socket_;
		socket_ = NULL;

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

	Listener *l = new Listener();

	event_main();

	delete l;
}
