#ifndef	XCODEC_WINDOW_H
#define	XCODEC_WINDOW_H

#include <map>

#define	XCODEC_WINDOW_MAX		(0xff)
#define	XCODEC_WINDOW_COUNT		(XCODEC_WINDOW_MAX + 1)

/*
 * XXX
 * Make more like an LRU and make present() bump up in the window.
 */
class XCodecWindow {
	uint64_t window_[XCODEC_WINDOW_COUNT];
	unsigned cursor_;
	std::map<uint64_t, unsigned> present_;
	std::map<uint64_t, BufferSegment *> segments_;
public:
	XCodecWindow(void)
	: window_(),
	  cursor_(0),
	  present_(),
	  segments_()
	{
		unsigned b;
		
		for (b = 0; b < XCODEC_WINDOW_COUNT; b++) {
			window_[b] = 0;
		}
	}

	~XCodecWindow()
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
			
			std::map<uint64_t, BufferSegment *>::iterator it;
			it = segments_.find(old);
			ASSERT(it != segments_.end());
			BufferSegment *oseg = it->second;
			oseg->unref();
			segments_.erase(it);
		}

		window_[cursor_] = hash;
		present_[hash] = cursor_;
		seg->ref();
		segments_[hash] = seg;
		cursor_ = (cursor_ + 1) % XCODEC_WINDOW_COUNT;
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

#endif /* !XCODEC_WINDOW_H */
