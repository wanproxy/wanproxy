/*
 * Copyright (c) 2008-2014 Juli Mallett. All rights reserved.
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
	typedef	std::pair<unsigned, BufferSegment *> cursor_segment_t;

	uint64_t window_[XCODEC_WINDOW_COUNT];
	unsigned cursor_;
	std::map<uint64_t, cursor_segment_t> present_;
public:
	XCodecWindow(void)
	: window_(),
	  cursor_(0),
	  present_()
	{
		unsigned b;

		for (b = 0; b < XCODEC_WINDOW_COUNT; b++) {
			window_[b] = 0;
		}
	}

	~XCodecWindow()
	{
		std::map<uint64_t, cursor_segment_t>::iterator it;

		for (it = present_.begin(); it != present_.end(); ++it)
			it->second.second->unref();
		present_.clear();
	}

	bool declare(uint64_t hash, BufferSegment *seg)
	{
		std::map<uint64_t, cursor_segment_t>::iterator it;
		bool collision;

		ASSERT("/xcodec/window", hash != 0);

		it = present_.find(hash);
		collision = it != present_.end();
		if (collision) {
			it->second.second->unref();
			window_[it->second.first] = 0;
			present_.erase(it);
		}

		uint64_t old = window_[cursor_];
		if (old != 0) {
			it = present_.find(old);
			ASSERT("/xcodec/window", it != present_.end());
			ASSERT("/xcodec/window", it->second.first == cursor_);
			BufferSegment *oseg = it->second.second;
			oseg->unref();
			present_.erase(it);
		}

		window_[cursor_] = hash;
		seg->ref();
		present_[hash] = cursor_segment_t(cursor_, seg);
		cursor_ = (cursor_ + 1) % XCODEC_WINDOW_COUNT;

		return (collision);
	}

	BufferSegment *dereference(unsigned c) const
	{
		if (window_[c] == 0)
			return (NULL);
		std::map<uint64_t, cursor_segment_t>::const_iterator it;
		it = present_.find(window_[c]);
		ASSERT("/xcodec/window", it != present_.end());
		BufferSegment *seg = it->second.second;
		seg->ref();
		return (seg);
	}

	bool present(uint64_t hash, const uint8_t *data, uint8_t *c) const
	{
		std::map<uint64_t, cursor_segment_t>::const_iterator it;
		it = present_.find(hash);
		if (it == present_.end())
			return (false);
		if (data != NULL && !it->second.second->equal(data, XCODEC_SEGMENT_LENGTH))
			return (false);
		ASSERT("/xcodec/window", window_[it->second.first] == hash);
		*c = it->second.first;
		return (true);
	}
};

#endif /* !XCODEC_XCODEC_WINDOW_H */
