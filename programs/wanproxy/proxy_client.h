#ifndef	PROXY_CLIENT_H
#define	PROXY_CLIENT_H

class ProxyPeer;
class TCPClient;
class XCodec;

class ProxyClient {
	LogHandle log_;
	Action *action_;
	XCodec *remote_codec_;
	ProxyPeer *local_peer_;
	TCPClient *remote_client_;
	ProxyPeer *remote_peer_;

public:
	ProxyClient(XCodec *, XCodec *, Channel *, const std::string&, unsigned);
	ProxyClient(XCodec *, XCodec *, Channel *, uint32_t, uint16_t);
private:
	~ProxyClient();

public:
	void close_peer(ProxyPeer *);
	ProxyPeer *get_peer(ProxyPeer *);

private:
	void connect_complete(Event, void *);
};

#endif /* !PROXY_CLIENT_H */
