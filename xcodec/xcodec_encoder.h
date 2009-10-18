#ifndef	XCODEC_ENCODER_H
#define	XCODEC_ENCODER_H

#include <xcodec/xcodec_window.h>

class XCodecCache;
class XCodecEncoderPipe;

class XCodecEncoder {
	LogHandle log_;
	XCodecCache *cache_;
	XCodecWindow window_;
	XCodecEncoderPipe *pipe_;
	Buffer queued_;

public:
	XCodecEncoder(XCodec *, XCodecEncoderPipe *);
	~XCodecEncoder();

	void encode(Buffer *, Buffer *);

	void encode_ask(uint64_t);
	void encode_learn(BufferSegment *);
private:
	void encode_declaration(Buffer *, Buffer *, unsigned, uint64_t, BufferSegment **);
	void encode_escape(Buffer *, Buffer *, unsigned);
	bool encode_reference(Buffer *, Buffer *, unsigned, uint64_t, BufferSegment *);
};

#endif /* !XCODEC_ENCODER_H */
