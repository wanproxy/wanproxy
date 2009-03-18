#ifndef	XCODEC_ENCODER_H
#define	XCODEC_ENCODER_H

#include <set>

#include <xcodec/xcbackref.h>

class XCDatabase;

class XCodecEncoder {
	struct Data {
		Buffer prefix_;
		uint64_t hash_;
		BufferSegment *seg_;

		Data(void);
		Data(const Data&);
		~Data();
	};

	LogHandle log_;
	XCDatabase *database_;
	XCBackref backref_;

public:
	XCodecEncoder(XCodec *);
	~XCodecEncoder();

	void encode(Buffer *, Buffer *);
};

#endif /* !XCODEC_ENCODER_H */
