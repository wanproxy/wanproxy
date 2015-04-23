/*
 * Copyright (c) 2013-2015 Juli Mallett. All rights reserved.
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

#ifndef	XCODEC_XCODEC_CACHE_DISK_H
#define	XCODEC_XCODEC_CACHE_DISK_H

class XCodecDiskCache;

/*
 * This handles the actual on-disk data, shared by
 * many front-ends, which store their own indices.
 */
class XCodecDisk {
	LogHandle log_;

	int fd_;

	uint64_t disk_blocks_;
	uint64_t index_blocks_;

	std::map<uint16_t, XCodecDiskCache *> xuid_cache_map_;
	std::map<UUID, uint16_t> uuid_xuid_map_;

	uint64_t current_index_block_;
	Buffer index_block_;
	size_t index_block_next_;
	uint64_t index_block_counter_;

	XCodecDisk(int, uint64_t);

	~XCodecDisk()
	{
		ASSERT(log_, fd_ == -1);
		ASSERT(log_, xuid_cache_map_.empty());
	}

	uint64_t data_block_address(uint64_t, unsigned);

	uint64_t index_block_address(uint64_t);
	bool index_invalidate_entries(uint64_t);
	bool index_load_entries(uint64_t, bool);
	bool index_read_counter(uint64_t, uint64_t *);

	bool registry_collect(void);
	bool registry_load(void);
	bool registry_write(uint16_t, const Buffer *);

public:
	XCodecDiskCache *connect(const UUID&);
	XCodecDiskCache *local(void);

	void enter(XCodecDiskCache *, uint64_t, const BufferSegment *);
	BufferSegment *lookup(XCodecDiskCache *, uint64_t);
	void remove(XCodecDiskCache *, uint64_t);
	void touch(XCodecDiskCache *, uint64_t, const BufferSegment *);

	static XCodecDisk *open(const std::string&, uint64_t);
};

/*
 * This is a front-end to the XCodec disk cache.
 *
 * Each front-end is associated with a single UUID,
 * while the on-disk backend is shared by all UUIDs
 * using it.
 *
 * Each front-end stores the in-memory state
 * associated with its UUID.
 */
class XCodecDiskCache : public XCodecCache {
	friend class XCodecDisk;

	typedef __gnu_cxx::hash_map<Tag64, uint64_t> hash_cache_t;

	LogHandle log_;
	XCodecDisk *disk_;
	hash_cache_t hash_cache_;
	uint16_t xuid_;

	XCodecDiskCache(const UUID& uuid, XCodecDisk *disk, uint16_t xuid)
	: XCodecCache(uuid),
	  log_("/xcodec/cache/disk"),
	  disk_(disk),
	  hash_cache_(),
	  xuid_(xuid)
	{ }

	~XCodecDiskCache()
	{ }

public:
	XCodecCache *connect(const UUID& uuid)
	{
		return (disk_->connect(uuid));
	}

	void enter(const uint64_t& hash, BufferSegment *seg)
	{
		disk_->enter(this, hash, seg);
	}

	void replace(const uint64_t& hash, BufferSegment *seg)
	{
		disk_->remove(this, hash);
		disk_->enter(this, hash, seg);
	}

	bool out_of_band(void) const
	{
		/*
		 * Disk caches are not exchanged out-of-band; references
		 * must be extracted in-stream.
		 */
		return (false);
	}

	BufferSegment *lookup(const uint64_t& hash)
	{
		return (disk_->lookup(this, hash));
	}

	void touch(const uint64_t& hash, BufferSegment *seg)
	{
		disk_->touch(this, hash, seg);
	}
};

#endif /* !XCODEC_XCODEC_CACHE_DISK_H */
