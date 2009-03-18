#ifndef	XCODEC_SLICE_H
#define	XCODEC_SLICE_H

#include <set>

class XCBackref;
class XCDatabase;

class XCodecSlice {
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

	std::set<uint64_t> declarations_;
	std::vector<Data> data_;
	Buffer suffix_;
public:
	XCodecSlice(XCDatabase *, Buffer *);
	~XCodecSlice();

private:
	void process(Buffer *);

public:
	void encode(XCBackref *, Buffer *) const;
};

#endif /* !XCODEC_SLICE_H */
