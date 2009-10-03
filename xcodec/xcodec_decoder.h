#ifndef	XCODEC_DECODER_H
#define	XCODEC_DECODER_H

#include <xcodec/xcodec_window.h>

class XCodecCache;

class XCodecDecoder {
	LogHandle log_;
	XCodecCache *cache_;
	XCodecWindow window_;
	uint64_t input_bytes_;
	uint64_t output_bytes_;

public:
	XCodecDecoder(XCodec *);
	~XCodecDecoder();

	bool decode(Buffer *, Buffer *);
};

#endif /* !XCODEC_DECODER_H */
