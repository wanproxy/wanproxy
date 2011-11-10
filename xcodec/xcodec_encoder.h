#ifndef	XCODEC_ENCODER_H
#define	XCODEC_ENCODER_H

#include <xcodec/xcodec_window.h>

class XCodecCache;

class XCodecEncoder {
	LogHandle log_;
	XCodecCache *cache_;
	XCodecWindow window_;
	bool stream_;

public:
	XCodecEncoder(XCodecCache *);
	~XCodecEncoder();

	void encode(Buffer *, Buffer *);
private:
	void encode_declaration(Buffer *, Buffer *, unsigned, uint64_t, BufferSegment **);
	void encode_escape(Buffer *, Buffer *, unsigned);
	bool encode_reference(Buffer *, Buffer *, unsigned, uint64_t, BufferSegment *);
};

#endif /* !XCODEC_ENCODER_H */
