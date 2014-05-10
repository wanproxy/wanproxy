/*
 * Copyright (c) 2008-2013 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	XCODEC_XCODEC_WINDOW_H
#define	XCODEC_XCODEC_WINDOW_H

#include <map>

#define	XCODEC_WINDOW_MAX		(0xff)
#define	XCODEC_WINDOW_COUNT		(XCODEC_WINDOW_MAX + 1)

/*
 * XXX
 * Make more like an LRU and make present() bump up in the window.
 *
 * Maybe add an explicit use() mechanism?
 */
class XCodecWindow {
	uint64_t window_[XCODEC_WINDOW_COUNT];
	unsigned cursor_;
	/*
	 * XXX
	 * Combine these two maps.  This is wasteful.
	 */
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
			ASSERT("/xcodec/window", present_[old] == cursor_);
			present_.erase(old);

			std::map<uint64_t, BufferSegment *>::iterator it;
			it = segments_.find(old);
			ASSERT("/xcodec/window", it != segments_.end());
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
		if (window_[c] == 0)
			return (NULL);
		std::map<uint64_t, BufferSegment *>::const_iterator it;
		it = segments_.find(window_[c]);
		ASSERT("/xcodec/window", it != segments_.end());
		BufferSegment *seg = it->second;
		seg->ref();
		return (seg);
	}

	/*
	 * XXX
	 * We need to pass in the BufferSegment here, too, so that we can
	 * verify that the copy in the window by hash has the same contents
	 * as the hash being referred to by the caller.  If we switch to
	 * generation numbers, that will just need to be used here alongside
	 * the hash as well.
	 */
	bool present(uint64_t hash, uint8_t *c) const
	{
		std::map<uint64_t, unsigned>::const_iterator it = present_.find(hash);
		if (it == present_.end())
			return (false);
		ASSERT("/xcodec/window", window_[it->second] == hash);
		*c = it->second;
		return (true);
	}
};

#endif /* !XCODEC_XCODEC_WINDOW_H */
