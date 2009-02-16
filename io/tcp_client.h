#ifndef	TCP_CLIENT_H
#define	TCP_CLIENT_H

class TCPClient : public Channel {
	LogHandle log_;
	Socket *socket_;

	TCPClient(Socket *socket)
	: log_("/tcp/client"),
	  socket_(socket)
	{ }

public:
	~TCPClient()
	{
		if (socket_ != NULL) {
			delete socket_;
			socket_ = NULL;
		}
	}

	Action *close(EventCallback *cb)
	{
		return (socket_->close(cb));
	}

	Action *read(EventCallback *cb)
	{
		return (socket_->read(cb));
	}

	Action *write(Buffer *buf, EventCallback *cb)
	{
		return (socket_->write(buf, cb));
	}

	static Action *connect(TCPClient **, const std::string&, int,
			       EventCallback *);
};

#endif /* !TCP_CLIENT_H */
