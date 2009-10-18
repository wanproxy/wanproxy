#ifndef	XCODEC_DECODER_H
#define	XCODEC_DECODER_H

#include <xcodec/xcodec_window.h>

class XCodecCache;
class XCodecEncoder;

class XCodecDecoder {
	LogHandle log_;
	XCodecCache *cache_;
	XCodecWindow window_;
	XCodecEncoder *encoder_;
	bool parsing_queued_;
	Buffer queued_;

public:
	XCodecDecoder(XCodec *, XCodecEncoder *);
	~XCodecDecoder();

	bool decode(Buffer *, Buffer *);
};

#endif /* !XCODEC_DECODER_H */
