#ifndef	PROXY_PEER_H
#define	PROXY_PEER_H

class ProxyClient;
class XCodec;
class XCodecDecoder;
class XCodecEncoder;

class ProxyPeer {
	friend class ProxyClient;

	LogHandle log_;
	ProxyClient *client_;
	Channel *channel_;
	XCodec *codec_;
	XCodecDecoder *decoder_;
	XCodecEncoder *encoder_;

	Action *close_action_;
	Action *read_action_;
	Buffer read_buffer_;
	Action *write_action_;
	Buffer write_buffer_;

	bool closed_;
	bool error_;
public:
	ProxyPeer(const LogHandle&, ProxyClient *, Channel *, XCodec *);
	~ProxyPeer();

	void start(void);

private:
	void close_complete(Event, void *);
	void read_complete(Event, void *);
	void write_complete(Event, void *);

	void schedule_close(void);
	void schedule_read(void);
	void schedule_write(void);
	void schedule_write(Buffer *);
	void schedule_write(ProxyPeer *);
};

#endif /* !PROXY_PEER_H */
