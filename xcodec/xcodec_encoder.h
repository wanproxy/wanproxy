#ifndef	XCODEC_ENCODER_H
#define	XCODEC_ENCODER_H

#include <xcodec/xcodec_window.h>

class XCodecCache;
class XCodecDecoder;
#if defined(XCODEC_PIPES)
class XCodecEncoderPipe;
#endif

class XCodecEncoder {
#if defined(XCODEC_PIPES)
	friend class XCodecEncoderPipe;
#endif

	LogHandle log_;
	XCodecCache *cache_;
	XCodecWindow window_;
#if defined(XCODEC_PIPES)
	XCodecEncoderPipe *pipe_;
#endif
	Buffer queued_;
	bool sent_eos_;
	bool received_eos_; /* XXX So wrong!  */

public:
	XCodecEncoder(XCodec *);
	~XCodecEncoder();

	void encode(Buffer *, Buffer *);

	void encode_ask(uint64_t);
	void encode_learn(BufferSegment *);
	void encode_push(void);

	void received_eos(void);

#if defined(XCODEC_PIPES)
	void set_pipe(XCodecEncoderPipe *);
#endif
private:
	void encode_declaration(Buffer *, Buffer *, unsigned, uint64_t, BufferSegment **);
	void encode_escape(Buffer *, Buffer *, unsigned);
	bool encode_reference(Buffer *, Buffer *, unsigned, uint64_t, BufferSegment *);
};

#endif /* !XCODEC_ENCODER_H */
