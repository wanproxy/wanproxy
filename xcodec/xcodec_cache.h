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
struct Hash64 {
	uint64_t hash_;

	Hash64(const uint64_t& hash)
	: hash_(hash)
	{ }

	bool operator== (const Hash64& hash) const
	{
		return (hash_ == hash.hash_);
	}

	bool operator< (const Hash64& hash) const
	{
		return (hash_ < hash.hash_);
	}
};

namespace __gnu_cxx {
	template<>
	struct hash<Hash64> {
		size_t operator() (const Hash64& x) const
		{
			return (x.hash_);
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
	virtual BufferSegment *lookup(const uint64_t&) const = 0;
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
	typedef __gnu_cxx::hash_map<Hash64, BufferSegment *> segment_hash_map_t;

	LogHandle log_;
	segment_hash_map_t segment_hash_map_;
public:
	XCodecMemoryCache(const UUID& uuid)
	: XCodecCache(uuid),
	  log_("/xcodec/cache/memory"),
	  segment_hash_map_()
	{ }

	~XCodecMemoryCache()
	{
		segment_hash_map_t::const_iterator it;
		for (it = segment_hash_map_.begin();
		     it != segment_hash_map_.end(); ++it)
			it->second->unref();
		segment_hash_map_.clear();
	}

	void enter(const uint64_t& hash, BufferSegment *seg)
	{
		ASSERT(log_, seg->length() == XCODEC_SEGMENT_LENGTH);
		ASSERT(log_, segment_hash_map_.find(hash) == segment_hash_map_.end());
		seg->ref();
		segment_hash_map_[hash] = seg;
	}

	bool out_of_band(void) const
	{
		/*
		 * Memory caches are not exchanged out-of-band; references
		 * must be extracted in-stream.
		 */
		return (false);
	}

	BufferSegment *lookup(const uint64_t& hash) const
	{
		segment_hash_map_t::const_iterator it;
		it = segment_hash_map_.find(hash);
		if (it == segment_hash_map_.end())
			return (NULL);

		BufferSegment *seg;

		seg = it->second;
		seg->ref();
		return (seg);
	}
};

#endif /* !XCODEC_XCODEC_CACHE_H */
