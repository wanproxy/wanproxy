/*
 * Copyright (c) 2008-2015 Juli Mallett. All rights reserved.
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

#ifndef	XCODEC_XCODEC_LRU_H
#define	XCODEC_XCODEC_LRU_H

#include <map>

template<typename Tk>
class XCodecLRU {
	/*
	 * XXX
	 * We use a 64-bit counter/tag/timestamp for the LRU.  This ought
	 * to be enough for anyone, but hey; we should handle collisions
	 * more gracefully all the same.  Or use a 32-bit counter and
	 * handle wraps explicitly.
	 *
	 * At 1 billion entriess per second, it'd take half a millenium
	 * for us to need to care about the 64-bit counter wrapping.  So
	 * we needn't worry about that.  But perhaps it's better to use
	 * a 32-bit counter and handle wraps all the same?
	 *
	 * NB
	 * For now we're using an ordered map to get the sorted behaviour,
	 * but we could also use an unordered (i.e. hash) map and keep
	 * track of the lowest counter in the LRU pretty easily.  The
	 * lookups would be quicker and the counter overhead minimal.
	 */

	typedef	std::map<uint64_t, Tk> counter_key_map_t;

	LogHandle log_;
	counter_key_map_t counter_key_map_;
	uint64_t lru_counter_;
public:
	XCodecLRU(void)
	: log_("/xcodec/lru"),
	  counter_key_map_(),
	  lru_counter_(0)
	{ }

	~XCodecLRU()
	{ }

	size_t active(void) const
	{
		return (counter_key_map_.size());
	}

	uint64_t enter(Tk key)
	{
		uint64_t counter = ++lru_counter_;
		counter_key_map_[counter] = key;
		return (counter);
	}

	Tk evict(void)
	{
		typename counter_key_map_t::iterator lit = counter_key_map_.begin();
		ASSERT(log_, lit != counter_key_map_.end());
		Tk okey = lit->second;
		counter_key_map_.erase(lit);
		return (okey);
	}

	uint64_t use(Tk key, uint64_t counter)
	{
		if (counter == lru_counter_)
			return (counter);

		typename counter_key_map_t::iterator lit = counter_key_map_.find(counter);
		ASSERT(log_, lit != counter_key_map_.end());
		ASSERT(log_, lit->second == key);
		counter_key_map_.erase(lit);

		counter = ++lru_counter_;
		counter_key_map_[counter] = key;
		return (counter);
	}
};

#endif /* !XCODEC_XCODEC_LRU_H */
