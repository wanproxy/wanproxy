#ifndef	XCODEC_ENCODER_H
#define	XCODEC_ENCODER_H

#include <xcodec/xcodec_window.h>

class XCodecCache;

class XCodecEncoder {
	LogHandle log_;
	XCodecCache *cache_;
	XCodecWindow window_;

public:
	XCodecEncoder(XCodec *);
	~XCodecEncoder();

	void encode(Buffer *, Buffer *);
};

#endif /* !XCODEC_ENCODER_H */
