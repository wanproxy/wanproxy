#ifndef	TCP_CLIENT_H
#define	TCP_CLIENT_H

class TCPClient {
	LogHandle log_;
	SocketAddressFamily family_;
	Socket *socket_;

	Action *close_action_;

	Action *connect_action_;
	EventCallback *connect_callback_;

	TCPClient(SocketAddressFamily);
	~TCPClient();

	Action *connect(const std::string&, const std::string&, EventCallback *);
	void connect_cancel(void);
	void connect_complete(Event);

	void close_complete(Event);

public:
	static Action *connect(SocketAddressFamily, const std::string&, EventCallback *);
	static Action *connect(SocketAddressFamily, const std::string&, const std::string&, EventCallback *);
};

#endif /* !TCP_CLIENT_H */
