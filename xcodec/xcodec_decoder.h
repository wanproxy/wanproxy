#ifndef	XCODEC_DECODER_H
#define	XCODEC_DECODER_H

#include <set>

#include <xcodec/xcodec_window.h>

class XCodecCache;
#if defined(XCODEC_PIPES)
class XCodecDecoderPipe;
#endif
class XCodecEncoder;

class XCodecDecoder {
#if defined(XCODEC_PIPES)
	friend class XCodecDecoderPipe;
#endif

	LogHandle log_;
	XCodecCache *cache_;
	XCodecWindow window_;
	XCodecEncoder *encoder_;
	Buffer queued_;
	std::set<uint64_t> asked_;

public:
	XCodecDecoder(XCodec *);
	~XCodecDecoder();

	bool decode(Buffer *, Buffer *);

	void set_encoder(XCodecEncoder *);
};

#endif /* !XCODEC_DECODER_H */
