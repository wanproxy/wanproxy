#ifndef	XCBACKREF_H
#define	XCBACKREF_H

#include <map>

#define	XCBACKREF_COUNT		(0xff)

class XCBackref {
	uint64_t window_[XCBACKREF_COUNT];
	unsigned cursor_;
	std::map<uint64_t, unsigned> present_;
	std::map<uint64_t, BufferSegment *> segments_;
public:
	XCBackref(void)
	: window_(),
	  cursor_(0),
	  present_(),
	  segments_()
	{
		unsigned b;
		
		for (b = 0; b < XCBACKREF_COUNT; b++) {
			window_[b] = 0;
		}
	}

	~XCBackref()
	{
		std::map<uint64_t, BufferSegment *>::iterator it;

		for (it = segments_.begin(); it != segments_.end(); ++it)
			it->second->unref();
		segments_.clear();
	}

	void declare(uint64_t hash, BufferSegment *seg)
	{
		if (hash == 0)
			return;

		if (present_.find(hash) != present_.end())
			return;

		uint64_t old = window_[cursor_];
		if (old != 0) {
			ASSERT(present_[old] == cursor_);
			present_.erase(old);
			segments_.erase(old);
		}

		window_[cursor_] = hash;
		present_[hash] = cursor_;
		seg->ref();
		segments_[hash] = seg;
		cursor_ = (cursor_ + 1) % XCBACKREF_COUNT;
	}

	BufferSegment *dereference(unsigned c) const
	{
		std::map<uint64_t, BufferSegment *>::const_iterator it;
		it = segments_.find(window_[c]);
		ASSERT(it != segments_.end());
		BufferSegment *seg = it->second;
		seg->ref();
		return (seg);
	}

	bool present(uint64_t hash, uint8_t *c) const
	{
		std::map<uint64_t, unsigned>::const_iterator it = present_.find(hash);
		if (it == present_.end())
			return (false);
		ASSERT(window_[it->second] == hash);
		*c = it->second;
		return (true);
	}
};

#endif /* !XCBACKREF_H */
