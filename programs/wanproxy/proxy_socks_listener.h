#ifndef	PROXY_SOCKS_LISTENER_H
#define	PROXY_SOCKS_LISTENER_H

class TCPServer;
class XCodec;

class ProxySocksListener {
	LogHandle log_;
	std::string name_;
	TCPServer *server_;
	Action *accept_action_;
	Action *close_action_;
	Action *stop_action_;
	std::string interface_;

public:
	ProxySocksListener(const std::string&, SocketAddressFamily, const std::string&);
	~ProxySocksListener();

private:
	void accept_complete(Event);
	void close_complete(void);
	void stop(void);
};

#endif /* !PROXY_SOCKS_LISTENER_H */
