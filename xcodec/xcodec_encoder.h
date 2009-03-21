#ifndef	XCODEC_ENCODER_H
#define	XCODEC_ENCODER_H

#include <xcodec/xcodec_window.h>

class XCodecCache;

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
	XCodecCache *cache_;
	XCodecWindow window_;

public:
	XCodecEncoder(XCodec *);
	~XCodecEncoder();

	void encode(Buffer *, Buffer *);
};

#endif /* !XCODEC_ENCODER_H */
