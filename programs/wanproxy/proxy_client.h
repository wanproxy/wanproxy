#ifndef	PROXY_CLIENT_H
#define	PROXY_CLIENT_H

#include <set>

class Pipe;
class PipePair;
class Splice;
class SplicePair;
class XCodec;

class ProxyClient {
	LogHandle log_;

	Action *stop_action_;

	Action *local_action_;
	XCodec *local_codec_;
	Socket *local_socket_;

	Action *remote_action_;
	XCodec *remote_codec_;
	Socket *remote_socket_;

	std::set<Pipe *> pipes_;
	std::set<PipePair *> pipe_pairs_;

	Splice *incoming_splice_;
	Splice *outgoing_splice_;
	SplicePair *splice_pair_;
	Action *splice_action_;

public:
	ProxyClient(XCodec *, XCodec *, Socket *, SocketAddressFamily, const std::string&);
private:
	~ProxyClient();

	void close_complete(Event, void *);
	void connect_complete(Event);
	void splice_complete(Event);
	void stop(void);

	void schedule_close(void);
};

#endif /* !PROXY_CLIENT_H */
