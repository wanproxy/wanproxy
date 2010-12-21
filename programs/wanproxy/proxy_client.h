#ifndef	PROXY_CLIENT_H
#define	PROXY_CLIENT_H

#include <set>

class Pipe;
class PipePair;
class Splice;
class SplicePair;

class ProxyClient {
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
	ProxyClient(const std::string&, PipePair *, Socket *, SocketAddressFamily, const std::string&);
private:
	~ProxyClient();

	void close_complete(Event, void *);
	void connect_complete(Event);
	void splice_complete(Event);
	void stop(void);

	void schedule_close(void);
};

#endif /* !PROXY_CLIENT_H */
