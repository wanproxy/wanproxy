#ifndef	XCODEC_DECODER_PIPE_H
#define	XCODEC_DECODER_PIPE_H

class Action;
class EventCallback;
class XCodecEncoder;

class XCodecDecoderPipe : public Pipe {
	friend class XCodecPipePair;

	XCodecDecoder decoder_;

	Buffer input_buffer_;
	bool input_eos_;

	Action *output_action_;
	EventCallback *output_callback_;
public:
	XCodecDecoderPipe(XCodec *);
	~XCodecDecoderPipe();

	Action *input(Buffer *, EventCallback *);
	Action *output(EventCallback *);

private:
	void output_cancel(void);
};

#endif /* !XCODEC_DECODER_PIPE_H */
