#ifndef	TCP_SERVER_H
#define	TCP_SERVER_H

class TCPServer {
	LogHandle log_;
	Socket *socket_;

	TCPServer(Socket *socket)
	: log_("/tcp/server"),
	  socket_(socket)
	{ }

public:
	~TCPServer()
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

	Action *close(EventCallback *cb)
	{
		return (socket_->close(cb));
	}

	static TCPServer *listen(const std::string&, unsigned *);
};

#endif /* !TCP_SERVER_H */
