#ifndef	UDP_CLIENT_H
#define	UDP_CLIENT_H

class UDPClient {
	LogHandle log_;
	SocketAddressFamily family_;
	Socket *socket_;

	Action *close_action_;

	Action *connect_action_;
	EventCallback *connect_callback_;

	UDPClient(SocketAddressFamily);
	~UDPClient();

	Action *connect(const std::string&, const std::string&, EventCallback *);
	void connect_cancel(void);
	void connect_complete(Event);

	void close_complete(Event);

public:
	static Action *connect(SocketAddressFamily, const std::string&, EventCallback *);
	static Action *connect(SocketAddressFamily, const std::string&, const std::string&, EventCallback *);
};

#endif /* !UDP_CLIENT_H */
