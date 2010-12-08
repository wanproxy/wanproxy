#ifndef	XCODEC_ENCODER_PIPE_H
#define	XCODEC_ENCODER_PIPE_H

#include <io/pipe/pipe_simple.h>

class XCodecEncoderPipe : public PipeSimple {
	friend class XCodecPipePair;

	XCodecEncoder encoder_;
public:
	XCodecEncoderPipe(XCodec *);
	~XCodecEncoderPipe();

	/* Only call from XCodecEncoder.  */
	void output_ready(void);

private:
	bool process(Buffer *, Buffer *);
};

#endif /* !XCODEC_ENCODER_PIPE_H */
