#ifndef	IO_SOCKET_SIMPLE_SERVER_H
#define	IO_SOCKET_SIMPLE_SERVER_H

#include <io/socket/socket.h>

/*
 * XXX
 * This is just one level up from using macros.  Would be nice to use abstract
 * base classes and something a bit tidier.
 */
template<typename A, typename C, typename L>
class SimpleServer {
	LogHandle log_;
	A arg_;
	L *server_;
	Action *accept_action_;
	Action *close_action_;
	Action *stop_action_;
public:
	SimpleServer(LogHandle log, A arg, SocketAddressFamily family, const std::string& interface)
	: log_(log),
	  arg_(arg),
	  server_(NULL),
	  accept_action_(NULL),
	  close_action_(NULL),
	  stop_action_(NULL)
	{
		server_ = L::listen(family, interface);
		if (server_ == NULL)
			HALT(log_) << "Unable to create listener.";

		INFO(log_) << "Listening on: " << server_->getsockname();

		EventCallback *cb = callback(this, &SimpleServer::accept_complete);
		accept_action_ = server_->accept(cb);

		Callback *scb = callback(this, &SimpleServer::stop);
		stop_action_ = EventSystem::instance()->register_interest(EventInterestStop, scb);
	}

	~SimpleServer()
	{
		ASSERT(server_ == NULL);
		ASSERT(accept_action_ == NULL);
		ASSERT(close_action_ == NULL);
		ASSERT(stop_action_ == NULL);
	}

private:
	void accept_complete(Event e)
	{
		accept_action_->cancel();
		accept_action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		case Event::Error:
			ERROR(log_) << "Accept error: " << e;
			break;
		default:
			ERROR(log_) << "Unexpected event: " << e;
			break;
		}

		if (e.type_ == Event::Done) {
			Socket *client = (Socket *)e.data_;

			INFO(log_) << "Accepted client: " << client->getpeername();

			new C(arg_, client);
		}

		EventCallback *cb = callback(this, &SimpleServer::accept_complete);
		accept_action_ = server_->accept(cb);
	}

	void close_complete(void)
	{
		close_action_->cancel();
		close_action_ = NULL;

		ASSERT(server_ != NULL);
		delete server_;
		server_ = NULL;

		delete this;
	}

	void stop(void)
	{
		stop_action_->cancel();
		stop_action_ = NULL;

		accept_action_->cancel();
		accept_action_ = NULL;

		ASSERT(close_action_ == NULL);

		Callback *cb = callback(this, &SimpleServer::close_complete);
		close_action_ = server_->close(cb);
	}
};

#endif /* !IO_SOCKET_SIMPLE_SERVER_H */
