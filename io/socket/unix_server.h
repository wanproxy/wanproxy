#ifndef	IO_SOCKET_UNIX_SERVER_H
#define	IO_SOCKET_UNIX_SERVER_H

#include <io/socket/socket.h>

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

	Action *accept(SocketEventCallback *cb)
	{
		return (socket_->accept(cb));
	}

	Action *close(SimpleCallback *cb)
	{
		return (socket_->close(cb));
	}

	static UnixServer *listen(const std::string&);
};

#endif /* !IO_SOCKET_UNIX_SERVER_H */
