#include <common/test.h>

#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/net/udp_client.h>
#include <io/net/udp_server.h>

#include <io/socket/socket.h>

static uint8_t data[1024];

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
		test_ = new Test(group_, "UDPClient::connect");
		EventCallback *cb =
			callback(this, &Connector::connect_complete);
		action_ = UDPClient::connect(family, remote, cb);
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

		ASSERT(socket_ != NULL);
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
			delete test_;
			test_ = NULL;
			return;
		}
		delete test_;
		test_ = NULL;

		Test _(group_, "UDPClient::connect set Socket pointer.");
		socket_ = (Socket *)e.data_;
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
	UDPServer *server_;
	Action *action_;
	Connector *connector_;
	Buffer read_buffer_;
public:
	Listener(const std::string& suffix, SocketAddressFamily family, const std::string& name)
	: log_("/listener"),
	  group_("/test/net/udp_server/listener" + suffix, "Socket listener"),
	  action_(NULL),
	  connector_(NULL),
	  read_buffer_()
	{
		{
			Test _(group_, "UDPServer::listen");
			server_ = UDPServer::listen(family, name);
			if (server_ == NULL)
				return;
			_.pass();
		}

		connector_ = new Connector(suffix, family, server_->getsockname());

		EventCallback *cb = callback(this, &Listener::read_complete);
		action_ = server_->read(cb);
	}

	~Listener()
	{
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

	void read_complete(Event e)
	{
		action_->cancel();
		action_ = NULL;

		{
			Test _(group_, "Socket read success");
			switch (e.type_) {
			case Event::Done:
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
		SimpleCallback *cb = callback(this, &Listener::close_complete);
		action_ = server_->close(cb);
	}

	void close_complete(void)
	{
		action_->cancel();
		action_ = NULL;

		ASSERT(server_ != NULL);
		delete server_;
		server_ = NULL;

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
