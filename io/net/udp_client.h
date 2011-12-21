#ifndef	IO_NET_UDP_CLIENT_H
#define	IO_NET_UDP_CLIENT_H

#include <io/socket/socket.h>

class UDPClient {
	LogHandle log_;
	SocketAddressFamily family_;
	Socket *socket_;

	Action *close_action_;

	Action *connect_action_;
	SocketEventCallback *connect_callback_;

	UDPClient(SocketAddressFamily);
	~UDPClient();

	Action *connect(const std::string&, const std::string&, SocketEventCallback *);
	void connect_cancel(void);
	void connect_complete(Event);

	void close_complete(void);

public:
	static Action *connect(SocketAddressFamily, const std::string&, SocketEventCallback *);
	static Action *connect(SocketAddressFamily, const std::string&, const std::string&, SocketEventCallback *);
};

#endif /* !IO_NET_UDP_CLIENT_H */
