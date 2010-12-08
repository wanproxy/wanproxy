#ifndef	UDP_SERVER_H
#define	UDP_SERVER_H

class UDPServer {
	LogHandle log_;
	Socket *socket_;

	UDPServer(Socket *socket)
	: log_("/udp/server"),
	  socket_(socket)
	{ }

public:
	~UDPServer()
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

	std::string getsockname(void) const
	{
		return (socket_->getsockname());
	}

	Action *read(EventCallback *cb)
	{
		return (socket_->read(0, cb));
	}

	static UDPServer *listen(SocketAddressFamily, const std::string&);
};

#endif /* !UDP_SERVER_H */
