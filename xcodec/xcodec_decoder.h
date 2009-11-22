#ifndef	XCODEC_DECODER_H
#define	XCODEC_DECODER_H

#include <set>

#include <xcodec/xcodec_window.h>

class XCodecCache;
class XCodecEncoder;
#if defined(XCODEC_PIPES)
class XCodecEncoderPipe;
#endif

class XCodecDecoder {
	LogHandle log_;
	XCodecCache *cache_;
	XCodecWindow window_;
	XCodecEncoder *encoder_;
#if defined(XCODEC_PIPES)
	XCodecEncoderPipe *encoder_pipe_;
	bool received_eos_;
#endif
	Buffer queued_;
	std::set<uint64_t> asked_;

public:
	XCodecDecoder(XCodec *);
	~XCodecDecoder();

	bool decode(Buffer *, Buffer *);

	void set_encoder(XCodecEncoder *);
#if defined(XCODEC_PIPES)
	void set_encoder_pipe(XCodecEncoderPipe *);
#endif
};

#endif /* !XCODEC_DECODER_H */
