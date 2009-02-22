#ifndef	PROXY_CLIENT_H
#define	PROXY_CLIENT_H

class ProxyPipe;
class TCPClient;
class XCodec;

class ProxyClient {
	LogHandle log_;

	Action *local_action_;
	XCodec *local_codec_;
	Channel *local_channel_;

	Action *remote_action_;
	XCodec *remote_codec_;
	TCPClient *remote_client_;

	Action *incoming_action_;
	ProxyPipe *incoming_pipe_;

	Action *outgoing_action_;
	ProxyPipe *outgoing_pipe_;

public:
	ProxyClient(XCodec *, XCodec *, Channel *, const std::string&, unsigned);
	ProxyClient(XCodec *, XCodec *, Channel *, uint32_t, uint16_t);
private:
	~ProxyClient();

	void close_complete(Event, void *);
	void connect_complete(Event, void *);
	void flow_complete(Event, void *);

	void schedule_close(void);
};

#endif /* !PROXY_CLIENT_H */
