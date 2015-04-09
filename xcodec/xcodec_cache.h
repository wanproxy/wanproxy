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

	virtual XCodecCache *connect(const UUID&) = 0;
	virtual void enter(const uint64_t&, BufferSegment *) = 0;
	virtual BufferSegment *lookup(const uint64_t&) = 0;
	virtual bool out_of_band(void) const = 0;

	bool uuid_encode(Buffer *buf) const
	{
		return (uuid_.encode(buf));
	}

	static XCodecCache *connect(const UUID& uuid, XCodecCache *parent)
	{
		XCodecCache *cache = lookup(uuid);
		if (cache != NULL)
			return (cache);
		cache = parent->connect(uuid);
		if (cache == NULL)
			return (NULL);
		enter(uuid, cache);
		return (cache);
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

class XCodecCacheLRU {
	/*
	 * XXX
	 * We use a 64-bit counter/tag/timestamp for the LRU.  This ought
	 * to be enough for anyone, but hey; we should handle collisions
	 * more gracefully all the same.  Or use a 32-bit counter and
	 * handle wraps explicitly.
	 *
	 * At 1 billion segments per second, it'd take half a millenium
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

	typedef	std::map<uint64_t, uint64_t> segment_lru_map_t;

	LogHandle log_;
	segment_lru_map_t segment_lru_map_;
	uint64_t segment_lru_counter_;
public:
	XCodecCacheLRU(void)
	: log_("/xcodec/cache/lru"),
	  segment_lru_map_(),
	  segment_lru_counter_(0)
	{ }

	~XCodecCacheLRU()
	{ }

	size_t active(void) const
	{
		return (segment_lru_map_.size());
	}

	uint64_t enter(uint64_t hash)
	{
		uint64_t counter = ++segment_lru_counter_;
		segment_lru_map_[counter] = hash;
		return (counter);
	}

	uint64_t evict(void)
	{
		segment_lru_map_t::iterator lit = segment_lru_map_.begin();
		ASSERT(log_, lit != segment_lru_map_.end());
		uint64_t ohash = lit->second;
		segment_lru_map_.erase(lit);
		return (ohash);
	}

	uint64_t use(uint64_t hash, uint64_t counter)
	{
		if (counter == segment_lru_counter_)
			return (counter);

		segment_lru_map_t::iterator lit = segment_lru_map_.find(counter);
		ASSERT(log_, lit != segment_lru_map_.end());
		ASSERT(log_, lit->second == hash);
		segment_lru_map_.erase(lit);

		counter = ++segment_lru_counter_;
		segment_lru_map_[counter] = hash;
		return (counter);
	}
};

class XCodecCachePair : public XCodecCache {
	XCodecCache *primary_;
	XCodecCache *secondary_;
public:
	XCodecCachePair(const UUID& uuid, XCodecCache *primary, XCodecCache *secondary)
	: XCodecCache(uuid),
	  primary_(primary),
	  secondary_(secondary)
	{ }

	~XCodecCachePair()
	{ }

	XCodecCache *connect(const UUID& uuid)
	{
		return (new XCodecCachePair(uuid, primary_->connect(uuid), secondary_->connect(uuid)));
	}

	void enter(const uint64_t& hash, BufferSegment *seg)
	{
		/*
		 * XXX
		 * We could also allow the caller to define policy here:
		 * do we want to spill evicted hashes from primary to
		 * secondary storage, or enter them into both?  For now,
		 * entering into both provides the most desirable
		 * behaviour for the cases we care about.
		 *
		 * Other policies here might be interesting, particularly
		 * if we ever have want of a more complex cache architecture,
		 * such as a unique per-pipe memory cache, then a cache
		 * per-peer, then a cache shared across multiple peers.  For
		 * now, that is not how we operate.
		 *
		 * Would be wise to split the UUID out of the cache at some
		 * point, too, and associate that with the name instead of
		 * the cache per se.
		 */
		primary_->enter(hash, seg);
		secondary_->enter(hash, seg);
	}

	bool out_of_band(void) const
	{
		if (primary_->out_of_band()) {
			ASSERT("/xcodec/cache/pair", secondary_->out_of_band());
			return (true);
		}
		ASSERT("/xcodec/cache/pair", !secondary_->out_of_band());
		return (false);
	}

	BufferSegment *lookup(const uint64_t& hash)
	{
		BufferSegment *seg = primary_->lookup(hash);
		if (seg == NULL) {
			seg = secondary_->lookup(hash);
			if (seg != NULL)
				primary_->enter(hash, seg);
		}
		return (seg);
	}
};

/*
 * XXX
 * Would be easy enough to rename this something like BackendMemoryCache
 * and have the front-end handle sharing the backend on connect(), with
 * just a little additional key space.
 */
class XCodecMemoryCache : public XCodecCache {
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

	LogHandle log_;
	segment_hash_map_t segment_hash_map_;
	XCodecCacheLRU segment_lru_;
	size_t memory_cache_limit_;
public:
	XCodecMemoryCache(const UUID& uuid, size_t memory_cache_limit_bytes = 0)
	: XCodecCache(uuid),
	  log_("/xcodec/cache/memory"),
	  segment_hash_map_(),
	  segment_lru_(),
	  memory_cache_limit_(memory_cache_limit_bytes / XCODEC_SEGMENT_LENGTH)
	{
		if (memory_cache_limit_bytes != 0 &&
		    memory_cache_limit_bytes < XCODEC_SEGMENT_LENGTH)
			memory_cache_limit_ = 1;
	}

	~XCodecMemoryCache()
	{ }

	/*
	 * Just assume that we can spin up another memory cache of the same size
	 * for any connecting UUID.
	 */
	XCodecCache *connect(const UUID& uuid)
	{
		XCodecMemoryCache *cache = new XCodecMemoryCache(uuid, memory_cache_limit_ * XCODEC_SEGMENT_LENGTH);
		return (cache);
	}

	void enter(const uint64_t& hash, BufferSegment *seg)
	{
		if (memory_cache_limit_ != 0 &&
		    segment_lru_.active() == memory_cache_limit_) {
			/*
			 * Find the oldest hash.
			 */
			uint64_t ohash = segment_lru_.evict();

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
		if (memory_cache_limit_ != 0)
			entry.counter_ = segment_lru_.enter(hash);
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
		segment_hash_map_t::iterator it;
		it = segment_hash_map_.find(hash);
		if (it == segment_hash_map_.end())
			return (NULL);

		CacheEntry& entry = it->second;
		/*
		 * If we have a limit and if we aren't also the last hash to
		 * be used, update our position in the LRU.
		 */
		if (memory_cache_limit_ != 0)
			entry.counter_ = segment_lru_.use(hash, entry.counter_);
		entry.seg_->ref();
		return (entry.seg_);
	}
};

#endif /* !XCODEC_XCODEC_CACHE_H */
