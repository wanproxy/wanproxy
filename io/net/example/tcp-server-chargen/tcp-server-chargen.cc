#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#include <io/net/tcp_server.h>

#include <io/socket/simple_server.h>

/*
 * XXX
 * Register stop interest.
 */

#define	RFC864_STRING	"!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~ "
#define	RFC864_LEN	(sizeof RFC864_STRING - 1)
#define	WIDTH		72

class ChargenClient {
	LogHandle log_;
	Socket *client_;
	Action *action_;
	Buffer data_;
public:
	ChargenClient(Socket *client)
	: log_("/chargen/client"),
	  client_(client),
	  action_(NULL),
	  data_()
	{
		unsigned i;

		for (i = 0; i < RFC864_LEN; i++) {
			size_t resid = WIDTH;

			if (RFC864_LEN - i >= resid) {
				data_.append((const uint8_t *)&RFC864_STRING[i], resid);
			} else {
				data_.append((const uint8_t *)&RFC864_STRING[i], RFC864_LEN - i);
				resid -= RFC864_LEN - i;
				data_.append((const uint8_t *)RFC864_STRING, resid);
			}
			data_.append("\r\n");
		}

		INFO(log_) << "Client connected: " << client_->getpeername();

		EventCallback *cb = callback(this, &ChargenClient::shutdown_complete);
		action_ = client_->shutdown(true, false, cb);
	}

	~ChargenClient()
	{
		ASSERT(client_ == NULL);
		ASSERT(action_ == NULL);
	}

	void close_complete(void)
	{
		action_->cancel();
		action_ = NULL;

		ASSERT(client_ != NULL);
		delete client_;
		client_ = NULL;

		delete this;
	}

	void shutdown_complete(Event e)
	{
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		case Event::Error:
			ERROR(log_) << "Error during shutdown: " << e;
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		schedule_write();
	}

	void write_complete(Event e)
	{
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		case Event::Error:
			ERROR(log_) << "Error during write: " << e;
			schedule_close();
			return;
		default:
			HALT(log_) << "Unexpected event: " << e;
			schedule_close();
			return;
		}

		schedule_write();
	}

	void schedule_close(void)
	{
		ASSERT(action_ == NULL);
		ASSERT(client_ != NULL);

		SimpleCallback *cb = callback(this, &ChargenClient::close_complete);
		action_ = client_->close(cb);
	}

	void schedule_write(void)
	{
		ASSERT(action_ == NULL);
		ASSERT(client_ != NULL);

		Buffer tmp(data_);
		EventCallback *cb = callback(this, &ChargenClient::write_complete);
		action_ = client_->write(&tmp, cb);
	}
};

class ChargenListener : public SimpleServer<TCPServer> {
public:
	ChargenListener(void)
	: SimpleServer<TCPServer>("/chargen/listener", SocketAddressFamilyIP, "[::]:0")
	{ }

	~ChargenListener()
	{ }

	void client_connected(Socket *socket)
	{
		new ChargenClient(socket);
	}
};


int
main(void)
{
	new ChargenListener();

	event_main();
}
