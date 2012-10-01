#ifndef	PROGRAMS_WANPROXY_SSH_PROXY_CONNECTOR_H
#define	PROGRAMS_WANPROXY_SSH_PROXY_CONNECTOR_H

#include <set>

#include <io/channel.h>

#include <ssh/ssh_session.h>
#include <ssh/ssh_transport_pipe.h>

#include "ssh_stream.h"

class Pipe;
class PipePair;
class Socket;
class Splice;
class SplicePair;
struct WANProxyCodec;

class SSHProxyConnector {
	LogHandle log_;

	Action *stop_action_;

	Action *local_action_;
	Socket *local_socket_;

	Action *remote_action_;
	Socket *remote_socket_;

	PipePair *pipe_pair_;

	SSHStream incoming_stream_;
	Action *incoming_stream_action_;
	Pipe *incoming_pipe_;
	Splice *incoming_splice_;

	SSHStream outgoing_stream_;
	Action *outgoing_stream_action_;
	Pipe *outgoing_pipe_;
	Splice *outgoing_splice_;

	SplicePair *splice_pair_;
	Action *splice_action_;
public:
	SSHProxyConnector(const std::string&, PipePair *, Socket *, SocketAddressFamily, const std::string&, WANProxyCodec *, WANProxyCodec *);
private:
	~SSHProxyConnector();

	void close_complete(Socket *);
	void connect_complete(Event, Socket *);
	void splice_complete(Event);
	void stop(void);

	void schedule_close(void);

	void ssh_stream_complete(SSHStream *);
};

#endif /* !PROGRAMS_WANPROXY_SSH_PROXY_CONNECTOR_H */
