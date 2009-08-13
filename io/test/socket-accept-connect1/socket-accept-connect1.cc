#include <common/buffer.h>
#include <common/test.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/file_descriptor.h>
#include <io/socket.h>

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

	void close_complete(Event e)
	{
		action_->cancel();
		action_ = NULL;

		{
			Test _(group_, "Socket closed successfully");
			if (e.type_ == Event::Done)
				_.pass();
		}
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

		EventCallback *cb = callback(this, &Connector::close_complete);
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
			if (!socket_->listen(1))
				return;
			_.pass();
		}

		connector_ = new Connector(socket_->getsockname());

		EventCallback *cb = callback(this, &Listener::accept_complete);
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

	void accept_complete(Event e)
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
			client_ = (Socket *)e.data_;
			if (client_ != NULL)
				_.pass();
		}

		EventCallback *cb = callback(this, &Listener::close_complete);
		action_ = socket_->close(cb);
	}

	void close_complete(Event e)
	{
		action_->cancel();
		action_ = NULL;

		{
			Test _(group_, "Socket closed successfully");
			if (e.type_ == Event::Done)
				_.pass();
		}
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
		EventCallback *cb = callback(this, &Listener::client_close);
		action_ = client_->close(cb);
	}

	void client_close(Event e)
	{
		action_->cancel();
		action_ = NULL;

		{
			Test _(group_, "Client socket closed successfully");
			if (e.type_ == Event::Done)
				_.pass();
		}
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
	EventSystem::instance()->start();
	delete l;
}
