#ifndef	PROXY_SOCKS_LISTENER_H
#define	PROXY_SOCKS_LISTENER_H

class FlowTable;
class TCPServer;
class XCodec;

class ProxySocksListener {
	LogHandle log_;
	FlowTable *flow_table_;
	TCPServer *server_;
	Action *action_;
	std::string interface_;
	unsigned local_port_;

public:
	ProxySocksListener(FlowTable *, const std::string&, unsigned);
	~ProxySocksListener();

private:
	void accept_complete(Event);
};

#endif /* !PROXY_SOCKS_LISTENER_H */
