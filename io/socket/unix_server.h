#ifndef	UNIX_SERVER_H
#define	UNIX_SERVER_H

class UnixServer {
	LogHandle log_;
	Socket *socket_;

	UnixServer(Socket *socket)
	: log_("/unix/server"),
	  socket_(socket)
	{ }

public:
	~UnixServer()
	{
		if (socket_ != NULL) {
			delete socket_;
			socket_ = NULL;
		}
	}

	Action *accept(EventCallback *cb)
	{
		return (socket_->accept(cb));
	}

	Action *close(SimpleCallback *cb)
	{
		return (socket_->close(cb));
	}

	static UnixServer *listen(const std::string&);
};

#endif /* !UNIX_SERVER_H */
