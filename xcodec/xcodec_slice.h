#ifndef	XCODEC_SLICE_H
#define	XCODEC_SLICE_H

class XCBackref;
class XCDatabase;

class XCodecSlice {
	enum Type {
		EscapeData,
		HashReference,
	};

	LogHandle log_;
	XCDatabase *database_;

	Type type_;

	Buffer prefix_;
	std::map<uint64_t, BufferSegment *> declarations_;
	uint64_t hash_;
	BufferSegment *seg_;
	std::deque<XCodecSlice *> children_;
	Buffer suffix_;
public:
	XCodecSlice(XCDatabase *, Buffer *);
private:
	XCodecSlice(XCDatabase *, Buffer *, uint64_t, BufferSegment *);
public:
	~XCodecSlice();

	void encode(XCBackref *, Buffer *) const;
	void process(Buffer *);
};

#endif /* !XCODEC_SLICE_H */
