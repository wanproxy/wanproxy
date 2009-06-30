#ifndef	PROXY_SOCKS_LISTENER_H
#define	PROXY_SOCKS_LISTENER_H

class TCPServer;
class XCodec;

class ProxySocksListener {
	LogHandle log_;
	TCPServer *server_;
	Action *accept_action_;
	Action *close_action_;
	Action *stop_action_;
	std::string interface_;
	unsigned local_port_;

public:
	ProxySocksListener(const std::string&, unsigned);
	~ProxySocksListener();

private:
	void accept_complete(Event);
	void close_complete(Event);
	void stop(void);
};

#endif /* !PROXY_SOCKS_LISTENER_H */
