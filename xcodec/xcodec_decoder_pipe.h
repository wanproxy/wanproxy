#ifndef	XCODEC_DECODER_PIPE_H
#define	XCODEC_DECODER_PIPE_H

#include <io/pipe_simple.h>

class XCodecDecoderPipe : public PipeSimple {
	friend class XCodecPipePair;

	XCodecDecoder decoder_;
public:
	XCodecDecoderPipe(XCodec *);
	~XCodecDecoderPipe();

private:
	bool process(Buffer *, Buffer *);
	bool process_eos(void) const;
};

#endif /* !XCODEC_DECODER_PIPE_H */
