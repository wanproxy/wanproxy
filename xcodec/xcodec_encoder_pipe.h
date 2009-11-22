#ifndef	XCODEC_ENCODER_PIPE_H
#define	XCODEC_ENCODER_PIPE_H

#include <io/pipe_simple.h>

class XCodecEncoderPipe : public PipeSimple {
	friend class XCodecPipePair;

	XCodecEncoder encoder_;

	bool sent_eos_;
	bool received_eos_;
public:
	XCodecEncoderPipe(XCodec *);
	~XCodecEncoderPipe();

	/* Only call from XCodecEncoder.  */
	void received_eos(bool);
	void output_ready(void);

private:
	bool process(Buffer *, Buffer *);
};

#endif /* !XCODEC_ENCODER_PIPE_H */
