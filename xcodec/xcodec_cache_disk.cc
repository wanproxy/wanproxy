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

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <common/buffer.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>
#include <xcodec/xcodec_cache_disk.h>
#include <xcodec/xcodec_hash.h>

/*
 * TODO
 *
 * It would be nice to mmap(2) the data blocks associated with the current
 * index block being written into memory, so that on insert we just have to
 * do a copy, rather than a write.  We would sync when the mapping changes,
 * which gives the same consistency guarantees as we have now.
 *
 * If we also were to mmap(2) the index blocks themselves, we could reduce
 * the amount of data written without having to care about the underlying
 * device block size, and of course this would also be great for the insert
 * case.  It would be nice if the index then had a format suitable for
 * in-memory traversal, but at present it certainly does not.
 */

/*
 * XXX
 * Move from UUIDs to public keys, and
 * identify ourselves with a private key
 * we associate with our cache.  Easy!
 * Better than UUIDs, also!  And solves
 * the authentication issue, or will with
 * minimal changes, and doesn't mean we
 * have to add extra identification,
 * it's just that a memory-only instance
 * REALLY is a different instance on each
 * start; we make guarantees about identity
 * AND knowledge.
 *
 * So the registry would begin with a
 * private key, and then public keys would
 * follow.
 */

#define	XCDFS_BLOCK_SIZE	(2048)		/* Same as segment size for Buffer and for XCodec.  */

/*
 * The index block layout is:
 * uint64_t counter; -- A monotonically increasing counter for each index block used,
 *                      so that we can order the index blocks on disk, discovering the
 *                      head and tail, and knowing which are the oldest (most likely to
 *                      have been overwritten) and newest (most likely to not be in sync)
 *                      so that we can check those thoroughly, which we do not want to
 *                      have to do for the whole disk.
 * uint16_t entry_0_xuid; -- The XUID associated with the corresponding data block.
 * uint64_t entry_0_hash; -- The XCodecHash of the corresponding data block.
 *  ...
 * [entries sufficient to fill the block.]
 */
#define	XCDFS_ENTRIES_PER_INDEX_BLOCK	((XCDFS_BLOCK_SIZE - sizeof (uint64_t)) / (sizeof (uint16_t) + sizeof (uint64_t)))

#define	XCDFS_XUID_LOCAL	(0)
#define	XCDFS_XUID_COUNT	(1024)

#define	XCDFS_REGISTRY_BLOCKS		((XCDFS_XUID_COUNT * UUID_SIZE) / XCDFS_BLOCK_SIZE)
#define	XCDFS_REGISTRY_BLOCK_ENTRIES	(XCDFS_BLOCK_SIZE / UUID_SIZE)

/*
 * We check the first 80 and last 80 index blocks during load.
 *
 * This corresponds to a typical disk cache size worth of data (64MB),
 * which is about what we might expect to need checking.
 */
#define	XCDFS_CHECK_BOUNDARY	(80)

/*
 * XXX
 * Want support for device block size being a multiple or divisor of the logical block size.
 */
namespace {
	static bool
	read_block(int fd, uint8_t block[XCDFS_BLOCK_SIZE], uint64_t blockno)
	{
		ssize_t amt = ::pread(fd, block, XCDFS_BLOCK_SIZE, blockno * XCDFS_BLOCK_SIZE);
		if (amt == -1)
			return (false);
		ASSERT("/xcodec/disk", amt == XCDFS_BLOCK_SIZE);
		return (true);
	}

	/*
	 * XXX
	 * Would be nice to only write the changed device-level block(s), rather
	 * than the whole XCDFS block.
	 */
	static bool
	write_block(int fd, const uint8_t block[XCDFS_BLOCK_SIZE], uint64_t blockno)
	{
		ssize_t amt = ::pwrite(fd, block, XCDFS_BLOCK_SIZE, blockno * XCDFS_BLOCK_SIZE);
		if (amt == -1)
			return (false);
		ASSERT("/xcodec/disk", amt == XCDFS_BLOCK_SIZE);
		return (true);
	}

	static uint8_t zero_uuid[UUID_SIZE];
}

XCodecDisk::XCodecDisk(int fd, uint64_t disk_size)
: log_("/xcodec/disk"),
  fd_(fd),
  disk_blocks_(disk_size / XCDFS_BLOCK_SIZE),
  index_blocks_((disk_blocks_ - XCDFS_REGISTRY_BLOCKS) / (1 + XCDFS_ENTRIES_PER_INDEX_BLOCK)),
  xuid_cache_map_(),
  uuid_xuid_map_(),
  current_index_block_(0),
  index_block_(),
  index_block_next_(0),
  index_block_counter_(0)
{
	uint64_t o;

	ASSERT(log_, XCDFS_BLOCK_SIZE == XCODEC_SEGMENT_LENGTH);

	DEBUG(log_) << "Opened disk with " << index_blocks_ << " index blocks.  Block size is " << XCDFS_BLOCK_SIZE << ".";
	DEBUG(log_) << "Disk maps " << (index_blocks_ * XCDFS_ENTRIES_PER_INDEX_BLOCK) << " data blocks.";
	DEBUG(log_) << "Using " << XCDFS_REGISTRY_BLOCKS << " registry blocks to map " << XCDFS_XUID_COUNT << " namespaces.";
	DEBUG(log_) << "Volume size " << disk_size << ".";
	DEBUG(log_) << "Actual size of store and metadata " << ((XCDFS_REGISTRY_BLOCKS + index_blocks_ + (XCDFS_ENTRIES_PER_INDEX_BLOCK * index_blocks_)) * XCDFS_BLOCK_SIZE) << ".";
	DEBUG(log_) << "Wasted space " << disk_size - ((XCDFS_REGISTRY_BLOCKS + index_blocks_ + (XCDFS_ENTRIES_PER_INDEX_BLOCK * index_blocks_)) * XCDFS_BLOCK_SIZE) << ".";

	if (index_blocks_ == 0) {
		ERROR(log_) << "Disk too small; reduce XCDFS_BLOCK_SIZE.";
		return;
	}

	if (!registry_load())
		HALT(log_) << "Could not load registry and cannot recover.";

	std::map<uint64_t, uint64_t> counter_index_map;
	for (o = 0; o < index_blocks_; o++) {
		uint64_t counter;
		if (!index_read_counter(o, &counter))
			HALT(log_) << "Could not read counter for index block #" << o << ".";
		if (counter == 0) {
			DEBUG(log_) << "Free index block found at index block #" << o << ".";
			counter_index_map[0] = o;
			break;
		}
		if (counter > index_block_counter_)
			index_block_counter_ = counter + 1;
		counter_index_map[counter] = o;
	}

	std::map<uint64_t, uint64_t>::const_iterator it;
	if ((it = counter_index_map.begin()) != counter_index_map.end()) {
		if (it->first == 0) {
			DEBUG(log_) << "Write head is at first free index block; index block #" << it->second << ".";
		} else {
			DEBUG(log_) << "Lowest counter found in index block #" << it->second << ".  This will be the write head.";
		}
		current_index_block_ = it->second;

		/*
		 * We do not want to load entries from the index we are
		 * about to erase.  There is no code to invalidate them
		 * once we begin writing.
		 */
		counter_index_map.erase(it);
	} else {
		DEBUG(log_) << "This appears to be an empty disk; writing from start.";
	}

	/*
	 * XXX
	 * We should detect ordering corruption also, unless we want to
	 * overwrite things in counter order and assign counters to free
	 * indices, and thereby be able to do something more like LRU
	 * replacement rather than FIFO.  This would actually be a small
	 * step, given our use of counters to detect FIFO head and tail
	 * already.
	 *
	 * We would just have a map (or other sorted queue) of counter
	 * to block, and assign the next counter to a block on use and
	 * on touch.  There is still the problem of getting rid of stale
	 * segments around hot ones, but that's easier; we could track
	 * the number of times a block gets its counter bumped and cap
	 * that in proportion to the size of the disk cache, forcing its
	 * entries to be rewritten on use or touch once the block itself
	 * has spoiled.  This would require no additional on-disk
	 * metadata, a number of more disk writes (which we could also
	 * cap proportionally), and only a little more in-memory
	 * overhead, since we can always let blocks warm back up once
	 * we have to restart.
	 *
	 * Even easier, though breaking with some of the current design
	 * decisions, would be to have the index pages include XUID,
	 * hash, and counter for each data block, and just LRU the data
	 * blocks themselves, rather than the index blocks.  This has a
	 * cost in terms of inserts to the index, but with the current
	 * consistency model, would not actually need to be much worse,
	 * especially if we mmap(2) the index blocks and let the kernel
	 * do the work there.
	 */
	unsigned leading = XCDFS_CHECK_BOUNDARY;
	while ((it = counter_index_map.begin()) != counter_index_map.end()) {
		/*
		 * We want to check the first N and last N index pages.
		 * These are the ones most likely to have not yet been
		 * synced, or to have been overwritten, and we need to
		 * discard any inconsistencies they may hold.
		 *
		 * By loading in counter order, we also ensure that we
		 * will use the newest version of each hash, and will
		 * evict a hash if it has been reused, but the reused
		 * one has been corrupted.
		 */
		bool need_check;
		if (leading != 0) {
			need_check = true;
			leading--;
		} else if (counter_index_map.size() <= XCDFS_CHECK_BOUNDARY) {
			need_check = true;
		} else {
			need_check = false;
		}

		if (!index_load_entries(it->second, need_check))
			HALT(log_) << "Could not load index block #" << o << ".";
		counter_index_map.erase(it);
	}

	if (!registry_collect())
		HALT(log_) << "Could not collect unused entries from registry.";

	if (index_block_counter_ == 0)
		index_block_counter_ = 1;
	index_block_.append(&index_block_counter_);
}

uint64_t
XCodecDisk::data_block_address(uint64_t index_block, unsigned entry)
{
	ASSERT(log_, index_block < index_blocks_);
	ASSERT(log_, entry < XCDFS_ENTRIES_PER_INDEX_BLOCK);
	return (XCDFS_REGISTRY_BLOCKS + index_blocks_ + (index_block * XCDFS_ENTRIES_PER_INDEX_BLOCK) + entry);
}

uint64_t
XCodecDisk::index_block_address(uint64_t index_block)
{
	ASSERT(log_, index_block < index_blocks_);
	return (XCDFS_REGISTRY_BLOCKS + index_block);
}

/*
 * Note that we do not invalidate on-disk, because we have no need to.
 *
 * If we exit with an index block which is invalid but not yet rewritten
 * (which is the common case, in fact), we need to be able to deal with
 * that.  Once we track the head and tail of the FIFO, it should be a
 * simple matter to check the last N indices and invalidate them.
 */
bool
XCodecDisk::index_invalidate_entries(uint64_t index_block)
{
	uint8_t block[XCDFS_BLOCK_SIZE];

	/*
	 * Read in the index block and invalidate all entries
	 * that are currently active and primary.
	 */
	if (!read_block(fd_, block, index_block_address(index_block))) {
		ERROR(log_) << "Could not read index to be invalidated.";
		return (false);
	}

	Buffer idx(block, sizeof block);

	uint64_t counter;
	idx.moveout(&counter);

	if (counter == 0) {
		DEBUG(log_) << "Skipping invalidate for free index.";
		return (true);
	}

	unsigned i;
	for (i = 0; i < XCDFS_ENTRIES_PER_INDEX_BLOCK; i++) {
		uint16_t xuid;
		idx.moveout(&xuid);

		uint64_t hash;
		idx.moveout(&hash);

		if (hash == 0)
			continue;

		std::map<uint16_t, XCodecDiskCache *>::const_iterator xcit;
		xcit = xuid_cache_map_.find(xuid);
		if (xcit == xuid_cache_map_.end())
			continue;
		XCodecDiskCache *cache = xcit->second;
		ASSERT(log_, cache != NULL);

		XCodecDiskCache::hash_cache_t::const_iterator hcit;
		hcit = cache->hash_cache_.find(hash);
		if (hcit == cache->hash_cache_.end()) {
			DEBUG(log_) << "Skipping invalidate for absent hash.";
			continue;
		}
		const uint64_t& offset = hcit->second;
		if (offset != data_block_address(index_block, i)) {
			DEBUG(log_) << "Skipping invalidate for old, inactive hash.";
			continue;
		}
		cache->hash_cache_.erase(hcit);
	}

	return (true);
}

bool
XCodecDisk::index_load_entries(uint64_t index_block, bool check)
{
	uint8_t block[XCDFS_BLOCK_SIZE];

	if (!read_block(fd_, block, index_block_address(index_block))) {
		ERROR(log_) << "Could not read index to be loaded.";
		return (false);
	}

	Buffer idx(block, sizeof block);

	uint64_t counter;
	idx.moveout(&counter);

	if (counter == 0) {
		ERROR(log_) << "Block became free during index load.";
		return (false);
	}

	unsigned i;
	for (i = 0; i < XCDFS_ENTRIES_PER_INDEX_BLOCK; i++) {
		uint16_t xuid;
		idx.moveout(&xuid);

		uint64_t hash;
		idx.moveout(&hash);

		if (hash == 0)
			continue;

		std::map<uint16_t, XCodecDiskCache *>::const_iterator xcit;
		xcit = xuid_cache_map_.find(xuid);
		if (xcit == xuid_cache_map_.end()) {
			/*
			 * A 0 XUID is valid, but also could be unused space.
			 */
			if (xuid != 0)
				INFO(log_) << "Bogus XUID found in index block: " << xuid;
			continue;
		}
		XCodecDiskCache *cache = xcit->second;
		ASSERT(log_, cache != NULL);

		XCodecDiskCache::hash_cache_t::const_iterator hcit;
		hcit = cache->hash_cache_.find(hash);
		if (hcit != cache->hash_cache_.end()) {
			/*
			 * If we use the cache as a circular buffer, it
			 * becomes important that we're starting from the
			 * head and working to the tail here, so that we
			 * have the appropriate latest copy, otherwise a
			 * name recycle is more likely.
			 *
			 * It'd be nice if we could meaningfully distribute
			 * cache invalidates to attached peers also, which
			 * we could, but currently do not have any kind of
			 * facility for.
			 */
			INFO(log_) << "Replacing previous cache entry.";
			cache->hash_cache_.erase(hcit);
		}

		const uint64_t& offset = data_block_address(index_block, i);

		if (check) {
			if (!read_block(fd_, block, offset)) {
				ERROR(log_) << "Could not read data entry for check.";
				return (false);
			}

			uint64_t ohash = XCodecHash::hash(block);
			if (ohash != hash) {
				INFO(log_) << "Removing invalid cache entry during check.";

				/*
				 * If we have an error, we should set the write head so
				 * that we will write over the corrupt blocks, since they
				 * are where we (assume) there has been the most recent
				 * activity that we have no desire to keep.
				 */
				if (current_index_block_ > index_block) {
					if (!index_invalidate_entries(index_block)) {
						ERROR(log_) << "Could not remove invalid index entries from block with errors.  Expect inconsistency.";
						return (false);
					}
					DEBUG(log_) << "Resetting current index to rewrite index block with errors.";
					current_index_block_ = index_block;
					return (true);
				}
				continue;
			}
		}

		cache->hash_cache_[hash] = offset;
	}

	return (true);
}

bool
XCodecDisk::index_read_counter(uint64_t index_block, uint64_t *counterp)
{
	uint8_t block[XCDFS_BLOCK_SIZE];

	if (!read_block(fd_, block, index_block_address(index_block))) {
		ERROR(log_) << "Could not read index to be loaded.";
		return (false);
	}

	Buffer idx(block, sizeof block);

	ASSERT(log_, counterp != NULL);
	idx.moveout(counterp);

	return (true);
}

bool
XCodecDisk::registry_collect(void)
{
	uint16_t xuid;

	for (xuid = 0; xuid < XCDFS_XUID_COUNT; xuid++) {
		std::map<uint16_t, XCodecDiskCache *>::const_iterator xcit;

		xcit = xuid_cache_map_.find(xuid);
		if (xcit == xuid_cache_map_.end())
			continue;
		XCodecDiskCache *cache = xcit->second;
		ASSERT(log_, cache != NULL);
		if (!cache->hash_cache_.empty()) {
			DEBUG(log_) << "Found " << cache->hash_cache_.size() << " entries in XUID #" << xuid << ".";
			continue;
		}

		/*
		 * Do not garbage collect the local XUID.
		 */
		if (xuid == XCDFS_XUID_LOCAL)
			continue;
		INFO(log_) << "XUID #" << xuid << " has no associated entries; removing entry.";
		Buffer uuidbuf(zero_uuid, sizeof zero_uuid);
		if (!registry_write(xuid, &uuidbuf)) {
			ERROR(log_) << "Could not clear unused XUID #" << xuid << ".";
			return (false);
		}

		uuid_xuid_map_.erase(cache->uuid_);
		xuid_cache_map_.erase(xcit);
		delete cache;
	}

	return (true);
}

bool
XCodecDisk::registry_load(void)
{
	uint8_t block[XCDFS_BLOCK_SIZE];
	unsigned i, r;

	for (r = 0; r < XCDFS_REGISTRY_BLOCKS; r++) {
		if (!read_block(fd_, block, r)) {
			ERROR(log_) << "Could not read registry block for loading.";
			return (false);
		}

		Buffer reg(block, sizeof block);

		for (i = 0; i < XCDFS_REGISTRY_BLOCK_ENTRIES; i++) {
			uint16_t xuid = r * XCDFS_REGISTRY_BLOCK_ENTRIES + i;

			Buffer uuidbuf;
			reg.moveout(&uuidbuf, UUID_SIZE);
			if (uuidbuf.equal(zero_uuid, sizeof zero_uuid))
				continue;

			UUID uuid;
			if (!uuid.decode(&uuidbuf)) {
				ERROR(log_) << "Could not decode XUID #" << xuid << ":" << std::endl << uuidbuf.hexdump();
				continue;
			}
			if (uuid_xuid_map_.find(uuid) != uuid_xuid_map_.end()) {
				ERROR(log_) << "Duplicate XUID #" << xuid << ":" << std::endl << uuidbuf.hexdump();
				continue;
			}

			XCodecDiskCache *cache = new XCodecDiskCache(uuid, this, xuid);
			xuid_cache_map_[xuid] = cache;
			uuid_xuid_map_[uuid] = xuid;
		}
	}

	/*
	 * Establish a local UUID if we do not have one.
	 */
	if (xuid_cache_map_.empty()) {
		UUID uuid;
		uuid.generate();

		Buffer uuidbuf;
		if (!uuid.encode(&uuidbuf)) {
			ERROR(log_) << "Could not encode local UUID for registry.";
			return (false);
		}

		if (!registry_write(XCDFS_XUID_LOCAL, &uuidbuf)) {
			ERROR(log_) << "Failed to write registry update for local UUID.";
			return (false);
		}

		XCodecDiskCache *cache = new XCodecDiskCache(uuid, this, XCDFS_XUID_LOCAL);
		xuid_cache_map_[XCDFS_XUID_LOCAL] = cache;
		uuid_xuid_map_[uuid] = XCDFS_XUID_LOCAL;
	}
	ASSERT(log_, xuid_cache_map_.find(XCDFS_XUID_LOCAL) != xuid_cache_map_.end());

	INFO(log_) << "Disk local UUID: " << xuid_cache_map_[XCDFS_XUID_LOCAL]->get_uuid().string_;

	return (true);
}

bool
XCodecDisk::registry_write(uint16_t xuid, const Buffer *uuidbuf)
{
	ASSERT(log_, xuid < XCDFS_XUID_COUNT);

	/*
	 * Read registry so we can update it.
	 */
	uint8_t block[XCDFS_BLOCK_SIZE];
	if (!read_block(fd_, block, xuid / XCDFS_REGISTRY_BLOCK_ENTRIES)) {
		ERROR(log_) << "Could not read registry block for update.";
		return (false);
	}

	uuidbuf->copyout(&block[(xuid % XCDFS_REGISTRY_BLOCK_ENTRIES) * UUID_SIZE], UUID_SIZE);

	if (!write_block(fd_, block, 0)) {
		ERROR(log_) << "Failed to write registry update.";
		return (false);
	}

	return (true);
}

XCodecDiskCache *
XCodecDisk::local(void)
{
	XCodecDiskCache *cache = xuid_cache_map_[XCDFS_XUID_LOCAL];
	ASSERT(log_, cache != NULL);
	return (cache);
}

XCodecDiskCache *
XCodecDisk::connect(const UUID& uuid)
{
	std::map<UUID, uint16_t>::const_iterator it;
	it = uuid_xuid_map_.find(uuid);
	if (it != uuid_xuid_map_.end()) {
		XCodecDiskCache *cache = xuid_cache_map_[it->second];
		ASSERT(log_, cache != NULL);
		return (cache);
	}

	uint16_t xuid;
	for (xuid = 0; xuid < XCDFS_XUID_COUNT; xuid++) {
		if (xuid_cache_map_.find(xuid) == xuid_cache_map_.end())
			break;
	}
	ASSERT(log_, xuid != XCDFS_XUID_LOCAL);
	if (xuid == XCDFS_XUID_COUNT) {
		if (!registry_collect()) {
			ERROR(log_) << "Failed to collect registry under XUID exhaustion; cannot connect new UUID.";
			return (NULL);
		}
		for (xuid = 0; xuid < XCDFS_XUID_COUNT; xuid++) {
			if (xuid_cache_map_.find(xuid) == xuid_cache_map_.end())
				break;
		}
		ASSERT(log_, xuid != XCDFS_XUID_LOCAL);
		if (xuid == XCDFS_XUID_COUNT) {
			ERROR(log_) << "Exhausted XUIDs for disk; cannot connect new UUID.";
			return (NULL);
		}
	}

	Buffer uuidbuf;
	if (!uuid.encode(&uuidbuf)) {
		ERROR(log_) << "Could not encode UUID for registry.";
		return (NULL);
	}

	if (!registry_write(xuid, &uuidbuf)) {
		ERROR(log_) << "Failed to write registry update for new UUID.";
		return (NULL);
	}

	XCodecDiskCache *cache = new XCodecDiskCache(uuid, this, xuid);
	xuid_cache_map_[xuid] = cache;
	uuid_xuid_map_[uuid] = xuid;
	return (cache);
}

void
XCodecDisk::enter(XCodecDiskCache *cache, uint64_t hash, const BufferSegment *seg)
{
	ASSERT(log_, cache->hash_cache_.find(hash) == cache->hash_cache_.end());

	index_block_.append(&cache->xuid_);
	index_block_.append(&hash);

	uint64_t offset = data_block_address(current_index_block_, index_block_next_);
	if (!write_block(fd_, seg->data(), offset)) {
		ERROR(log_) << "Could not write data segment.";
		return;
	}

	cache->hash_cache_[hash] = offset;

	if (++index_block_next_ == XCDFS_ENTRIES_PER_INDEX_BLOCK) {
		DEBUG(log_) << "Filled index block; writing to disk.";
		/*
		 * Filled index block, write it out.
		 *
		 * NB: We optimize for the common insert case at the cost of a
		 *     little consistency and to minimize the number of writes
		 *     we have to do in total.
		 */
		uint8_t block[XCDFS_BLOCK_SIZE];
		ASSERT(log_, index_block_.length() == XCDFS_BLOCK_SIZE);
		index_block_.moveout(block, index_block_.length());
		if (!write_block(fd_, block, index_block_address(current_index_block_)))
			ERROR(log_) << "Failed to write index block update; expect inconsistency.";

		if (++current_index_block_ == index_blocks_)
			current_index_block_ = 0;
		index_block_next_ = 0;

		/*
		 * We are going to be rewriting the entries associated
		 * with the new index block; purge them from memory.
		 */
		if (!index_invalidate_entries(current_index_block_))
			ERROR(log_) << "Could not invalidate new index block; expect inconsistency.";

		/* A counter of 0 always indicates unused.  */
		if (++index_block_counter_ == 0)
			index_block_counter_ = 1;
		index_block_.append(&index_block_counter_);
	}
}

BufferSegment *
XCodecDisk::lookup(XCodecDiskCache *cache, uint64_t hash)
{
	XCodecDiskCache::hash_cache_t::const_iterator it;
	it = cache->hash_cache_.find(hash);
	if (it == cache->hash_cache_.end())
		return (NULL);

	const uint64_t& offset = it->second;
	ASSERT(log_, offset != 0);

	BufferSegment *seg = BufferSegment::create();
	if (!read_block(fd_, seg->head(), offset)) {
		seg->unref();
		ERROR(log_) << "Could not read segment from disk; removing index entry.";
		cache->hash_cache_.erase(it);
		return (NULL);
	}
	seg->set_length(XCODEC_SEGMENT_LENGTH);

	uint64_t ohash = XCodecHash::hash(seg->data());
	if (ohash != hash) {
		seg->unref();
		ERROR(log_) << "Hash mismatch on disk; removing index entry.";
		cache->hash_cache_.erase(it);
		return (NULL);
	}

	return (seg);
}

void
XCodecDisk::remove(XCodecDiskCache *cache, uint64_t hash)
{
	XCodecDiskCache::hash_cache_t::const_iterator hcit;
	hcit = cache->hash_cache_.find(hash);
	if (hcit == cache->hash_cache_.end()) {
		ERROR(log_) << "Cannot remove absent hash.";
		return;
	}

	cache->hash_cache_.erase(hcit);
}

/*
 * There are many other, cleverer hot-cache schemes we could use to keep
 * track of segments we probably want to write to disk over and over again,
 * or keep from evicting from the cache, but this is a simple enough model
 * that has the real benefits: we simply reenter into the cache anything
 * which the primary cache has been using, but which has become absent.
 *
 * In practice, this really gives the behaviour of even much harder schemes
 * to implement.  For instance, we might be concerned in this case about
 * an entry which is overwritten and then not used again before we exit.
 * A minor improvement might then be to have the primary cache touch all
 * of its entries (in LRU order) on exit.  Moreover, the probability that
 * we desperately need to keep an entry around but that it happens to lose
 * that race is marginal; we are likely to get a bite at the apple here
 * to restore it to its desired place.
 *
 * XXX It would be nice if we had some kind of secondary, integrity-related
 *     hash that we could use, so that we could verify that this isn't name
 *     reuse.  We could lazily populate the hashes of on-disk data, but the
 *     cost of calculating them for the segment here might not be trivial.
 *
 *     If we do ever move to using names instead of hashes, it might make
 *     sense to keep integrity and encoder hashes around, since we do need
 *     the latter for processing lookups and such, but may also be able to
 *     integrate and make use of the former as part of auxiliary data, not
 *     just something used by the on-disk cache, in other places.
 */
void
XCodecDisk::touch(XCodecDiskCache *cache, uint64_t hash, const BufferSegment *seg)
{
	/* Do nothing if this is already in the cache.  */
	if (cache->hash_cache_.find(hash) != cache->hash_cache_.end())
		return;
	/*
	 * We have lost track of this entry, reenter it.
	 */
	enter(cache, hash, seg);
}

XCodecDisk *
XCodecDisk::open(const std::string& path, uint64_t size)
{
	static std::map<std::string, XCodecDisk *> disk_map;
	struct stat st;
	int fd;
	int rv;

	std::map<std::string, XCodecDisk *>::const_iterator it;
	it = disk_map.find(path);
	if (it != disk_map.end()) {
		INFO("/xcodec/disk") << "Multiple distinct opens of disk; disk will be shared.";
		return (it->second);
	}

	fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0700);
	if (fd == -1) {
		ERROR("/xcodec/disk") << "Could not open disk: " << path;
		return (NULL);
	}

	rv = fstat(fd, &st);
	if (rv == -1) {
		ERROR("/xcodec/disk") << "Could not stat disk.";
		close(fd);
		return (NULL);
	}

	if (size != 0 && S_ISREG(st.st_mode)) {
		rv = ftruncate(fd, size);
		if (rv == -1) {
			ERROR("/xcodec/disk") << "Could not truncate/extend disk.";
			close(fd);
			return (NULL);
		}
	}

	if (size == 0) {
		size = st.st_size;
		if (size == 0) {
			ERROR("/xcodec/disk") << "Could not determine disk size.";
			close(fd);
			return (NULL);
		}
	}

	XCodecDisk *disk = new XCodecDisk(fd, size);
	disk_map[path] = disk;
	return (disk);
}
