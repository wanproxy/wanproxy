#ifndef	PROGRAMS_WANPROXY_PROXY_CONNECTOR_H
#define	PROGRAMS_WANPROXY_PROXY_CONNECTOR_H

#include <set>

class Pipe;
class PipePair;
class Socket;
class Splice;
class SplicePair;

class ProxyConnector {
	LogHandle log_;

	Action *stop_action_;

	Action *local_action_;
	Socket *local_socket_;

	Action *remote_action_;
	Socket *remote_socket_;

	PipePair *pipe_pair_;

	Pipe *incoming_pipe_;
	Splice *incoming_splice_;

	Pipe *outgoing_pipe_;
	Splice *outgoing_splice_;

	SplicePair *splice_pair_;
	Action *splice_action_;

public:
	ProxyConnector(const std::string&, PipePair *, Socket *, SocketAddressFamily, const std::string&);
private:
	~ProxyConnector();

	void close_complete(Socket *);
	void connect_complete(Event, Socket *);
	void splice_complete(Event);
	void stop(void);

	void schedule_close(void);
};

#endif /* !PROGRAMS_WANPROXY_PROXY_CONNECTOR_H */
