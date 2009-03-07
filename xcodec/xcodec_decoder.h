#ifndef	XCODEC_DECODER_H
#define	XCODEC_DECODER_H

#include <xcodec/xcbackref.h>

class XCDatabase;

class XCodecDecoder {
	LogHandle log_;
	XCDatabase *database_;
	XCBackref backref_;

public:
	XCodecDecoder(XCodec *);
	~XCodecDecoder();

private:
	bool reference(Buffer *, Buffer *);
	bool unescape(Buffer *, Buffer *);
	bool declare(Buffer *);
	bool backreference(Buffer *, Buffer *);

public:
	bool decode(Buffer *, Buffer *);
};

#endif /* !XCODEC_DECODER_H */
