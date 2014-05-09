/*
 * Copyright (c) 2008-2011 Juli Mallett. All rights reserved.
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

#ifndef	XCODEC_XCODEC_CACHE_H
#define	XCODEC_XCODEC_CACHE_H

#include <ext/hash_map>
#include <map>

#include <common/uuid/uuid.h>

/*
 * XXX
 * GCC supports hash<unsigned long> but not hash<unsigned long long>.  On some
 * of our platforms, the former is 64-bits, on some the latter.  As a result,
 * we need this wrapper structure to throw our hashes into so that GCC's hash
 * function can be reliably defined to use them.
 */
struct Tag64 {
	uint64_t tag_;

	Tag64(const uint64_t& tag)
	: tag_(tag)
	{ }

	bool operator== (const Tag64& tag) const
	{
		return (tag_ == tag.tag_);
	}

	bool operator< (const Tag64& tag) const
	{
		return (tag_ < tag.tag_);
	}
};

namespace __gnu_cxx {
	template<>
	struct hash<Tag64> {
		size_t operator() (const Tag64& x) const
		{
			return (x.tag_);
		}
	};
}

class XCodecCache {
protected:
	UUID uuid_;

	XCodecCache(const UUID& uuid)
	: uuid_(uuid)
	{ }

public:
	virtual ~XCodecCache()
	{ }

	virtual void enter(const uint64_t&, BufferSegment *) = 0;
	virtual BufferSegment *lookup(const uint64_t&) = 0;
	virtual bool out_of_band(void) const = 0;

	bool uuid_encode(Buffer *buf) const
	{
		return (uuid_.encode(buf));
	}

	static void enter(const UUID& uuid, XCodecCache *cache)
	{
		ASSERT("/xcodec/cache", cache_map.find(uuid) == cache_map.end());
		cache_map[uuid] = cache;
	}

	static XCodecCache *lookup(const UUID& uuid)
	{
		std::map<UUID, XCodecCache *>::const_iterator it;

		it = cache_map.find(uuid);
		if (it == cache_map.end())
			return (NULL);

		return (it->second);
	}

private:
	static std::map<UUID, XCodecCache *> cache_map;
};

class XCodecMemoryCache : public XCodecCache {
	/*
	 * XXX
	 * We use a 64-bit counter/tag/timestamp for the LRU.  This ought
	 * to be enough for anyone, but hey; we should handle collisions
	 * more gracefully all the same.  Or use a 32-bit counter and
	 * handle wraps explicitly.
	 */
	struct CacheEntry {
		BufferSegment *seg_;
		uint64_t counter_;

		CacheEntry(BufferSegment *seg)
		: seg_(seg),
		  counter_(0)
		{
			seg_->ref();
		}

		CacheEntry(const CacheEntry& src)
		: seg_(src.seg_),
		  counter_(src.counter_)
		{
			seg_->ref();
		}

		~CacheEntry()
		{
			seg_->unref();
			seg_ = NULL;
		}
	};

	typedef __gnu_cxx::hash_map<Tag64, CacheEntry> segment_hash_map_t;
	typedef	__gnu_cxx::hash_map<Tag64, uint64_t> segment_lru_map_t;

	LogHandle log_;
	segment_hash_map_t segment_hash_map_;
	segment_lru_map_t segment_lru_map_;
	uint64_t segment_lru_counter_;
	size_t memory_cache_limit_;
public:
	XCodecMemoryCache(const UUID& uuid)
	: XCodecCache(uuid),
	  log_("/xcodec/cache/memory"),
	  segment_hash_map_(),
	  segment_lru_map_(),
	  segment_lru_counter_(0),
	  memory_cache_limit_(0)
	{ }

	XCodecMemoryCache(const UUID& uuid, size_t memory_cache_limit_bytes)
	: XCodecCache(uuid),
	  log_("/xcodec/cache/memory"),
	  segment_hash_map_(),
	  segment_lru_map_(),
	  segment_lru_counter_(0),
	  memory_cache_limit_(memory_cache_limit_bytes / XCODEC_SEGMENT_LENGTH)
	{ }

	~XCodecMemoryCache()
	{ }

	void enter(const uint64_t& hash, BufferSegment *seg)
	{
		if (memory_cache_limit_ != 0 &&
		    segment_hash_map_.size() == memory_cache_limit_) {
			/*
			 * Find the oldest hash.
			 */
			segment_lru_map_t::iterator lit = segment_lru_map_.begin();
			ASSERT(log_, lit != segment_lru_map_.end());
			uint64_t ohash = lit->second;
			segment_lru_map_.erase(lit);

			/*
			 * Remove the oldest hash.
			 */
			segment_hash_map_t::iterator oit = segment_hash_map_.find(ohash);
			ASSERT(log_, oit != segment_hash_map_.end());
			segment_hash_map_.erase(oit);
		}
		ASSERT(log_, seg->length() == XCODEC_SEGMENT_LENGTH);
		ASSERT(log_, segment_hash_map_.find(hash) == segment_hash_map_.end());
		CacheEntry entry(seg);
		if (memory_cache_limit_ != 0) {
			entry.counter_ = ++segment_lru_counter_;
			segment_lru_map_[entry.counter_] = hash;
		}
		segment_hash_map_.insert(segment_hash_map_t::value_type(hash, entry));
	}

	bool out_of_band(void) const
	{
		/*
		 * Memory caches are not exchanged out-of-band; references
		 * must be extracted in-stream.
		 */
		return (false);
	}

	BufferSegment *lookup(const uint64_t& hash)
	{
		segment_hash_map_t::const_iterator it;
		it = segment_hash_map_.find(hash);
		if (it == segment_hash_map_.end())
			return (NULL);

		CacheEntry& entry = it->second;
		/*
		 * If we have a limit and if we aren't also the last hash to
		 * be used, update our position in the LRU.
		 */
		if (memory_cache_limit_ != 0 && entry.counter_ != segment_lru_counter_) {
			segment_lru_map_t::iterator lit = segment_lru_map_.find(entry.counter_);
			ASSERT(log_, lit != segment_lru_map_.end());
			ASSERT(log_, lit->second == hash);
			segment_lru_map_.erase(lit);

			entry.counter_ = ++segment_lru_counter_;
			segment_lru_map_[entry.counter_] = hash;
		}

		entry.seg_->ref();
		return (entry.seg_);
	}
};

#endif /* !XCODEC_XCODEC_CACHE_H */
