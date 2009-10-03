#ifndef	XCODEC_ENCODER_H
#define	XCODEC_ENCODER_H

#include <xcodec/xcodec_window.h>

class XCodecCache;

class XCodecEncoder {
	LogHandle log_;
	XCodecCache *cache_;
	XCodecWindow window_;
	uint64_t input_bytes_;
	uint64_t output_bytes_;

public:
	XCodecEncoder(XCodec *);
	~XCodecEncoder();

	void encode(Buffer *, Buffer *);
private:
	void encode_declaration(Buffer *, Buffer *, unsigned, uint64_t, BufferSegment **);
	bool encode_reference(Buffer *, Buffer *, unsigned, uint64_t, BufferSegment *);
};

#endif /* !XCODEC_ENCODER_H */
