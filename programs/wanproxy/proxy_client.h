#ifndef	PROXY_CLIENT_H
#define	PROXY_CLIENT_H

class FlowTable;
class ProxyPipe;
class XCodec;

class ProxyClient {
	LogHandle log_;

	FlowTable *flow_table_;

	Action *stop_action_;

	Action *local_action_;
	XCodec *local_codec_;
	Socket *local_socket_;

	Action *remote_action_;
	XCodec *remote_codec_;
	Socket *remote_socket_;

	Action *incoming_action_;
	ProxyPipe *incoming_pipe_;

	Action *outgoing_action_;
	ProxyPipe *outgoing_pipe_;

public:
	ProxyClient(FlowTable *, XCodec *, XCodec *, Socket *, const std::string&, unsigned);
	ProxyClient(FlowTable *, XCodec *, XCodec *, Socket *, uint32_t, uint16_t);
private:
	~ProxyClient();

	void close_complete(Event, void *);
	void connect_complete(Event);
	void flow_complete(Event, void *);
	void stop(void);

	void schedule_close(void);
};

#endif /* !PROXY_CLIENT_H */
