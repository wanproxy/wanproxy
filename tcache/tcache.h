#ifndef	TCACHE_H
#define	TCACHE_H

#include <map>

template<typename K, size_t S>
class TCacheBackend {
public:
	TCacheBackend(void)
	{ }

	virtual ~TCacheBackend()
	{ }

	virtual bool read(K, uint8_t *) = 0;
	virtual bool write(K, const uint8_t *) = 0;
};

template<typename K>
class TCacheInterface {
public:
	TCacheInterface(void)
	{ }

	virtual ~TCacheInterface()
	{ }

	virtual bool read(K, uint8_t *, size_t) = 0;
	virtual bool write(K, const uint8_t *, size_t) = 0;
	virtual bool flush(void) = 0;
};

/**
 * K = Key type.
 * N = Number of cache lines.
 * P = TCache entries per cache line.
 * S = TCache entry size.
 */
template<typename K, unsigned N, unsigned P, size_t S>
class TCache : public TCacheInterface<K> {
	struct Index {
		K idx_;

		Index(const K& address)
		: idx_(address / (P * S))
		{ }

		~Index()
		{ }

		bool operator< (const Index& idx2) const
		{
			return (idx_ < idx2.idx_);
		}
	};

	struct Line {
		struct Entry {
			bool valid_;
			bool dirty_;
			uint8_t bytes_[S];
		};

		K address_;
		Entry entries_[P];
	};

	TCacheBackend<K, S> *backing_;
	std::map<Index, Line> lines_;
public:
	TCache(TCacheBackend<K, S> *backing)
	: backing_(backing),
	  lines_()
	{ }

	~TCache()
	{ }

	bool read(K address, uint8_t *dst, size_t len)
	{
		bool ok;

		/* XXX Prefetch.  */

		ok = move<false, uint8_t *>(address, dst, len);
		if (!ok) {
			return (false);
		}

		/* XXX Sync after read.  */

		return (true);
	}

	bool write(K address, const uint8_t *src, size_t len)
	{
		bool ok;

		ok = move<true, const uint8_t *>(address, src, len);
		if (!ok) {
			return (false);
		}

		/* XXX Sync after write.  */

		return (true);
	}

	bool flush(void)
	{
		bool ok;

		ok = sync();
		if (!ok) {
			return (false);
		}
		lines_.clear();
		return (true);
	}

	bool sync(void)
	{
		bool ok;

		typename std::map<Index, Line>::iterator it;
		for (it = lines_.begin(); it != lines_.end(); ++it) {
			Index idx = it->first;
			ok = flush(idx);
			if (!ok) {
				return (false);
			}
		}
		return (true);
	}

private:
	static size_t leader(K address, size_t len)
	{
		size_t l = ((P * S) - (address % (P * S))) % (P * S);
		if (l > len)
			return (len);
		return (l);
	}

	static size_t offset(K address)
	{
		size_t o = address % (P * S);
		return (o);
	}

	template<bool Write, typename Ta>
	bool move(K address, Ta p, size_t len)
	{
		size_t l = leader(address, len);
		bool ok;

		/* Fix up an unaligned start address.  */
		if (l != 0) {
			size_t o = offset(address);
			Index idx(address);

			ok = move_line<Write, Ta>(idx, p, o, l);
			if (!ok) {
				return (false);
			}

			address += l;
			p += l;
			len -= l;

			if (len != 0) {
				ok = move<Write, Ta>(address, p, len);
				if (!ok) {
					return (false);
				}
			}
			return (true);
		}

		while (len != 0) {
			Index idx(address);
			size_t amt = len < P * S ? len : P * S;

			ok = move_line<Write, Ta>(idx, p, 0, amt);
			if (!ok) {
				return (false);
			}

			address += amt;
			p += amt;
			len -= amt;
		}

		return (true);
	}

	template<bool Write, typename Ta>
	struct move_entry {
		bool operator() (typename Line::Entry&, Ta, unsigned, size_t);
	};

	template<typename Ta>
	struct move_entry<true, Ta> {
		/* Write.  */
		bool operator() (typename Line::Entry& e, Ta p, unsigned off,
				 size_t len)
		{
			/*
			 * Don't mark dirty if we're not changing anything.  If
			 * the entry is already dirty we might want to consider
			 * skipping the memcmp sicne the memmove is probably
			 * about as fast and we're not making any extra work for
			 * ourselves later...
			 */
			if (e.valid_) {
				if (memcmp(&e.bytes_[off], p, len) == 0) {
					return (true);
				}
			}
			memmove(&e.bytes_[off], p, len);
			e.dirty_ = true;
			return (true);
		}
	};

	template<typename Ta>
	struct move_entry<false, Ta> {
		/* Read.  */
		bool operator() (typename Line::Entry& e, Ta p, unsigned off,
				 size_t len)
		{
			if (!e.valid_) {
				return (false);
			}
			memmove(p, &e.bytes_[off], len);
			return (true);
		}
	};

	template<bool Write, typename Ta>
	bool move_line(Index idx, Ta p, unsigned off, size_t len)
	{
		bool ok;

		/*
		 * XXX
		 * We may want to allow writing direct to backing if it is not
		 * already in the cache.
		 */
		if (lines_.find(idx) == lines_.end()) {
			/*
			 * If the cache is full, remove one (randomly would be
			 * nice.
			 */
			if (lines_.size() == N) {
				ok = flush(lines_.begin()->first);
				if (!ok) {
					return (false);
				}
			}
		}

		/*
		 * It sure would be nice to be able to read or write an entire
		 * line in one go, but we'd have to make valid and dirty into
		 * bitmaps and store the data contiguously.
		 */
		unsigned i;
		for (i = 0; i < P; i++) {
			/*
			 * Within this entry?
			 */
			Line& line = lines_[idx];
			typename Line::Entry& e = line.entries_[i];
			if (len != 0 && off < S) {
				/*
				 * Doing a full write, we can skip backing.
				 */
				if (Write && off == 0 && len >= S) {
					move_entry<Write, Ta> move_entry_f;
					ok = move_entry_f(e, p, 0, S);
					if (!ok) {
						return (false);
					}
					e.valid_ = true;

					p += S;
					if (off != 0)
						off -= S;
					len -= S;
				} else {
					/*
					 * Partial write of this entry or a
					 * read of any size.
					 */
					ok = fetch(idx, i, e);
					if (!ok) {
						return (false);
					}

					size_t l = len < (S - off) ? len : S - off;

					move_entry<Write, Ta> move_entry_f;
					ok = move_entry_f(e, p, off, l);
					if (!ok) {
						return (false);
					}

					p += l;
					if (off != 0) {
						off = 0;
					}
					len -= l;
				}
			} else {
				/*
				 * Nothing to do for this entry.
				 *
				 * We could just leave this entry invalid if
				 * we're trying to minimize backend access.
				 */
				ok = fetch(idx, i, e);
				if (!ok) {
					return (false);
				}
				if (off != 0) {
					off -= S;
				}
			}
		}

		return (true);
	}

	/*
	 * XXX
	 * fetch and flush really need to be able to work on an entire line at
	 * a time, or pieces of it, so we can use readv and writev in cases
	 * where we have a lot of entries that are contiguous.
	 */

	bool fetch(Index idx, unsigned i, typename Line::Entry& e)
	{
		/*
		 * It would be nice to have a variant to fetch an entire line
		 * in a single I/O.  Ideally we'd be able to use a readv and
		 * the Operating System could combine the reads into a single
		 * block access.  Well, in the case where the cache is on top
		 * of a disk or file, obviously.  Other backings may have
		 * similar performance trade-offs to consider.
		 */
		if (e.valid_) {
			return (true);
		} else {
			K address = (idx.idx_ * (P * S)) + (i * S);
			bool ok;

			ok = backing_->read(address, e.bytes_);
			if (!ok) {
				return (false);
			}
			e.valid_ = true;

			return (true);
		}
	}

	bool flush(Index idx)
	{
		unsigned i;
		bool ok;

		if (lines_.find(idx) == lines_.end()) {
			return (false);
		}

		Line& line = lines_[idx];
		for (i = 0; i < P; i++) {
			typename Line::Entry& e = line.entries_[i];
			if (!e.valid_) {
				continue;
			}
			if (!e.dirty_) {
				continue;
			}

			K address = (idx.idx_ * (P * S)) + (i * S);

			ok = backing_->write(address, e.bytes_);
			if (!ok) {
				return (false);
			}
			e.dirty_ = false;

			e.valid_ = false;
		}

		return (true);
	}
};

#endif /* !TCACHE_H */
