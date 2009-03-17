#ifndef	XCODEC_ENCODER_H
#define	XCODEC_ENCODER_H

#include <xcodec/xcbackref.h>

class XCDatabase;

class XCodecEncoder {
	LogHandle log_;
	XCDatabase *database_;
	XCBackref backref_;

public:
	XCodecEncoder(XCodec *);
	~XCodecEncoder();

	void encode(Buffer *, Buffer *);
};

#endif /* !XCODEC_ENCODER_H */
