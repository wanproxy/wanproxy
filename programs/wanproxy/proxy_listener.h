#ifndef	PROXY_LISTENER_H
#define	PROXY_LISTENER_H

class TCPServer;
struct WANProxyCodec;

class ProxyListener {
	LogHandle log_;
	std::string name_;
	TCPServer *server_;
	Action *accept_action_;
	Action *close_action_;
	Action *stop_action_;
	WANProxyCodec *interface_codec_;
	std::string interface_;
	WANProxyCodec *remote_codec_;
	SocketAddressFamily remote_family_;
	std::string remote_name_;

public:
	ProxyListener(const std::string&, WANProxyCodec *, WANProxyCodec *, SocketAddressFamily,
		      const std::string&, SocketAddressFamily,
		      const std::string&);
	~ProxyListener();

private:
	void accept_complete(Event);
	void close_complete(Event);
	void stop(void);
};

#endif /* !PROXY_LISTENER_H */
