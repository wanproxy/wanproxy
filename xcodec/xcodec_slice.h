#ifndef	XCODEC_SLICE_H
#define	XCODEC_SLICE_H

#define	XCODEC_SLICE_SIZE	(XCODEC_SEGMENT_LENGTH * XCODEC_SEGMENT_LENGTH)

class XCBackref;
class XCDatabase;

class XCodecSlice {
	enum Type {
		EscapeData,
		HashDeclare,
		HashReference,
	};

	LogHandle log_;
	XCDatabase *database_;

	Type type_;

	Buffer prefix_;
	uint64_t hash_;
	BufferSegment *seg_;
	Buffer suffix_;
	XCodecSlice *next_;
public:
	XCodecSlice(XCDatabase *, Buffer *, XCodecSlice * = NULL);
	~XCodecSlice();

	void encode(XCBackref *, Buffer *) const;
	void process(Buffer *);
};

#endif /* !XCODEC_SLICE_H */
