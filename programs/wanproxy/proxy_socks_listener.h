#ifndef	PROXY_SOCKS_LISTENER_H
#define	PROXY_SOCKS_LISTENER_H

class TCPServer;
class XCodec;

class ProxySocksListener {
	LogHandle log_;
	TCPServer *server_;
	Action *action_;
	std::string interface_;
	unsigned local_port_;

public:
	ProxySocksListener(const std::string&, unsigned);
	~ProxySocksListener();

private:
	void accept_complete(Event, void *);
};

#endif /* !PROXY_SOCKS_LISTENER_H */
