#ifndef	PROGRAMS_WANPROXY_SSH_STREAM_H
#define	PROGRAMS_WANPROXY_SSH_STREAM_H

#include <io/channel.h>

#include <ssh/ssh_session.h>

class Socket;
class Splice;
namespace SSH { class TransportPipe; }
struct WANProxyCodec;

class SSHStream : public StreamChannel {
	LogHandle log_;
	Socket *socket_;
	SSH::Session session_;
	WANProxyCodec *incoming_codec_;
	WANProxyCodec *outgoing_codec_;
	SSH::TransportPipe *pipe_;
	Splice *splice_;
	Action *splice_action_;
	SimpleCallback *start_callback_;
	Action *start_action_;
	EventCallback *read_callback_;
	Action *read_action_;
	Buffer input_buffer_;
	bool ready_;
	SimpleCallback *ready_callback_;
	Action *ready_action_;
	EventCallback *write_callback_;
	Action *write_action_;
public:
	SSHStream(const LogHandle&, SSH::Role, WANProxyCodec *, WANProxyCodec *);
	~SSHStream();

	Action *start(Socket *socket, SimpleCallback *);

	Action *close(SimpleCallback *);
	Action *read(size_t, EventCallback *);
	Action *write(Buffer *, EventCallback *);
	Action *shutdown(bool, bool, EventCallback *);

private:
	void start_cancel(void);
	void read_cancel(void);

	void splice_complete(Event);

	void ready_complete(void);

	void receive_complete(Event);

	void write_cancel(void);
	void write_do(void);
};

#endif /* !PROGRAMS_WANPROXY_SSH_STREAM_H */
