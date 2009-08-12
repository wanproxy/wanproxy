#ifndef	PROXY_LISTENER_H
#define	PROXY_LISTENER_H

class TCPServer;
class XCodec;

class ProxyListener {
	LogHandle log_;
	TCPServer *server_;
	Action *accept_action_;
	Action *close_action_;
	Action *stop_action_;
	XCodec *local_codec_;
	XCodec *remote_codec_;
	std::string interface_;
	std::string remote_name_;

public:
	ProxyListener(XCodec *, XCodec *, const std::string&,
		      const std::string&);
	~ProxyListener();

private:
	void accept_complete(Event);
	void close_complete(Event);
	void stop(void);
};

#endif /* !PROXY_LISTENER_H */
