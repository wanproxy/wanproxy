#ifndef	PROXY_PIPE_H
#define	PROXY_PIPE_H

class XCodecDecoder;
class XCodecEncoder;

class ProxyPipe {
	LogHandle log_;

	Action *read_action_;
	Buffer read_buffer_;
	Channel *read_channel_;
	XCodecDecoder *read_decoder_;

	Action *write_action_;
	Buffer write_buffer_;
	Channel *write_channel_;
	XCodecEncoder *write_encoder_;

	Action *flow_action_;
	EventCallback *flow_callback_;
public:
	ProxyPipe(XCodec *, Channel *, Channel *, XCodec *);
	~ProxyPipe();

	Action *flow(EventCallback *);

private:
	void read_complete(Event, void *);
	void write_complete(Event, void *);

	void schedule_read(void);
	void schedule_write(void);

	void read_error(void);
	void write_error(void);

	void flow_close(void);
	void flow_error(void);
	void flow_cancel(void);
};

#endif /* !PROXY_PIPE_H */
