#ifndef	XCODEC_ENCODER_PIPE_H
#define	XCODEC_ENCODER_PIPE_H

class Action;
class EventCallback;

class XCodecEncoderPipe : public Pipe {
	friend class XCodecPipePair;

	XCodecEncoder encoder_;

	Buffer input_buffer_;
	bool input_eos_;

	Action *output_action_;
	EventCallback *output_callback_;
public:
	XCodecEncoderPipe(XCodec *);
	~XCodecEncoderPipe();

	Action *input(Buffer *, EventCallback *);
	Action *output(EventCallback *);

	/* Only call from XCodecEncoder.  */
	void output_ready(void);

private:
	void output_cancel(void);
};

#endif /* !XCODEC_ENCODER_PIPE_H */
