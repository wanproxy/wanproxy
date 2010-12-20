#ifndef	XCODEC_DECODER_H
#define	XCODEC_DECODER_H

#include <set>

#include <xcodec/xcodec_window.h>

class XCodecCache;

class XCodecDecoder {
	LogHandle log_;
	XCodecCache *cache_;
	XCodecWindow window_;

public:
	XCodecDecoder(XCodecCache *);
	~XCodecDecoder();

	bool decode(Buffer *, Buffer *, std::set<uint64_t>&);
};

#endif /* !XCODEC_DECODER_H */
