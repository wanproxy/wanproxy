#ifndef	TCP_CLIENT_H
#define	TCP_CLIENT_H

#include <io/socket/socket.h>

class TCPClient {
	LogHandle log_;
	SocketAddressFamily family_;
	Socket *socket_;

	Action *close_action_;

	Action *connect_action_;
	SocketEventCallback *connect_callback_;

	TCPClient(SocketAddressFamily);
	~TCPClient();

	Action *connect(const std::string&, const std::string&, SocketEventCallback *);
	void connect_cancel(void);
	void connect_complete(Event);

	void close_complete(void);

public:
	static Action *connect(SocketAddressFamily, const std::string&, SocketEventCallback *);
	static Action *connect(SocketAddressFamily, const std::string&, const std::string&, SocketEventCallback *);
};

#endif /* !TCP_CLIENT_H */
