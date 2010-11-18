#ifndef	BUFFER_H
#define	BUFFER_H

#include <string.h> /* memmove(3), memcpy(3), etc.  */

#include <deque>

struct iovec;

#define	BUFFER_SEGMENT_SIZE	(128)

typedef	uint8_t	buffer_segment_size_t;

/*
 * A BufferSegment is a contiguous chunk of data which may be at most a fixed
 * size of BUFFER_SEGMENT_SIZE.  The data in a BufferSegment may begin at a
 * non-zero offset within the BufferSegment and may end prematurely.  This
 * allows for skip()/trim() semantics.
 *
 * BufferSegments are reference counted and mutable operations copy before
 * doing a write.  You must provide your own locking or use only a single
 * thread.  One normally does not use BufferSegments directly unless one is
 * importing or exporting data in a performance-critical path.  Normal usage
 * uses a Buffer.
 */
class BufferSegment {
	uint8_t data_[BUFFER_SEGMENT_SIZE];
	buffer_segment_size_t offset_;
	buffer_segment_size_t length_;
	unsigned ref_;
public:
	/*
	 * Creates a new, empty BufferSegment with a single reference.
	 */
	BufferSegment(void)
	: data_(),
	  offset_(0),
	  length_(0),
	  ref_(1)
	{ }

	/*
	 * Should almost always only be called from unref().
	 */
	~BufferSegment()
	{
		ASSERT(ref_ == 0);
	}

	/*
	 * Bump the reference count.
	 */
	void ref(void)
	{
		ASSERT(ref_ != 0);
		ref_++;
	}

	/*
	 * Drop the reference count and delete if there are no other live
	 * references.
	 */
	void unref(void)
	{
		if (--ref_ == 0)
			delete this;
	}

	/*
	 * Return the number of outstanding references.
	 */
	unsigned refs(void) const
	{
		return (ref_);
	}

	/*
	 * Return a mutable pointer to the start of the data.  Discouraaged.
	 */
	uint8_t *head(void)
	{
		ASSERT(ref_ == 1);
		return (&data_[offset_]);
	}

	/*
	 * Return a mutable pointer to the point at which data may be
	 * appended.  Discouraged.
	 */
	uint8_t *tail(void)
	{
		ASSERT(ref_ == 1);
		return (&data_[offset_ + length_]);
	}

	/*
	 * Append data.
	 */
	BufferSegment *append(const uint8_t *buf, size_t len)
	{
		ASSERT(len <= avail());
		if (ref_ != 1) {
			BufferSegment *seg;

			seg = this->copy();
			this->unref();
			seg = seg->append(buf, len);
			return (seg);
		}
		if (avail() - offset_ < len)
			pullup();
		memmove(tail(), buf, len);
		length_ += len;
		return (this);
	}

	/*
	 * Append a character.
	 */
	BufferSegment *append(uint8_t ch)
	{
		return (append(&ch, 1));
	}

	/*
	 * Append a C++ string.
	 */
	BufferSegment *append(const std::string& str)
	{
		return (append((const uint8_t *)str.c_str(), str.length()));
	}

	/*
	 * Return the amount of data that is unused within the BufferSegment.
	 * This includes data before offset_.
	 */
	size_t avail(void) const
	{
		return (BUFFER_SEGMENT_SIZE - length());
	}

	/*
	 * Clone a BufferSegment.  Does a fixed-size copy to allow for more
	 * efficient code generation, but it is not strictly necessary.  Some
	 * reasons to believe that it would be better to start copied data
	 * with offset_ = 0, and to copy only length_ characters instead, but
	 * diverse microbenchmarking is essential.
	 */
	BufferSegment *copy(void) const
	{
		BufferSegment *seg = new BufferSegment();
		memcpy(seg->data_, data_, BUFFER_SEGMENT_SIZE);
		seg->offset_ = offset_;
		seg->length_ = length_;
		return (seg);
	}

	/*
	 * Copy out the requested number of bytes or the entire available
	 * length, whichever is smaller.  Returns the amount of data read.
	 */
	size_t copyout(uint8_t *dst, unsigned offset, size_t dstsize) const
	{
		size_t copied;

		if (offset + dstsize < length())
			copied = dstsize;
		else
			copied = length() - offset;
		memmove(dst, data() + offset, copied);
		return (copied);
	}

	/*
	 * Return a read-only pointer to the start of data in the segment.
	 */
	const uint8_t *data(void) const
	{
		return (&data_[offset_]);
	}

	/*
	 * Return a read-only pointer to the character after the end of the
	 * data.  If writable, this would be the point at which new data
	 * would be appended.
	 */
	const uint8_t *end(void) const
	{
		return (&data_[offset_ + length_]);
	}

	/*
	 * The length of this segment.
	 */
	size_t length(void) const
	{
		return (length_);
	}

	/*
	 * Moves data at an offset to the start of the BufferSegment.  Does not
	 * need to copy on write since data before offset_ is not reliable.
	 * Invalidates any direct pointers retrieved by head(), tail(), data()
	 * or end().
	 */
	void pullup(void)
	{
		if (offset_ == 0)
			return;
		memmove(data_, data(), length());
		offset_ = 0;
	}

	/*
	 * Manually sets the length of a BufferSegment that has had its data
	 * directly filled by e.g. writes through head() and tail().
	 */
	void set_length(size_t len)
	{
		ASSERT(ref_ == 1);
		ASSERT(offset_ == 0);
		ASSERT(len <= BUFFER_SEGMENT_SIZE);
		length_ = len;
	}

	/*
	 * Skip a number of bytes at the start of a BufferSemgent.  Creates a
	 * copy if there are other live references.
	 */
	BufferSegment *skip(unsigned bytes)
	{
		ASSERT(bytes < length());

		if (ref_ != 1) {
			BufferSegment *seg;

			seg = this->copy();
			this->unref();
			seg = seg->skip(bytes);
			return (seg);
		}
		offset_ += bytes;
		length_ -= bytes;

		return (this);
	}

	/*
	 * Adjusts the length to ignore bytes at the end of a BufferSegment.
	 * Like skip() but at the end rather than the start.  Creates a copy if
	 * there are live references.
	 */
	BufferSegment *trim(unsigned bytes)
	{
		ASSERT(bytes < length());

		if (ref_ != 1) {
			BufferSegment *seg;

			seg = this->copy();
			this->unref();
			seg = seg->trim(bytes);
			return (seg);
		}
		length_ -= bytes;

		return (this);
	}

	/*
	 * Adjusts the length to ignore bytes at the end of a BufferSegment.
	 * Like trim() but takes the desired resulting length rather than the
	 * number of bytes to trim.  Creates a copy if there are live
	 * references.
	 */
	BufferSegment *truncate(size_t len)
	{
		return (trim(length() - len));
	}

	/*
	 * Checks if the BufferSegment's contents are identical to the byte
	 * buffer passed in.
	 */
	bool match(const uint8_t *buf, size_t len) const
	{
		if (len != length())
			return (false);
		return (prefix(buf, len));
	}

	/*
	 * Checks if the contents of two BufferSegments are identical.
	 */
	bool match(const BufferSegment *seg) const
	{
		return (match(seg->data(), seg->length()));
	}

	/*
	 * Checks if the BufferSegment's contents are identical to the C++
	 * string passed in.
	 */
	bool match(const std::string& str) const
	{
		return (match((const uint8_t *)str.c_str(), str.length()));
	}

	/*
	 * Checks if the byte buffer occurs at the beginning of the
	 * BufferSegment in question.
	 */
	bool prefix(const uint8_t *buf, size_t len) const
	{
		/* Can't be a prefix if it's longer than the BufferSegment.  */
		if (len > length())
			return (false);
		return (memcmp(data(), buf, len) == 0);
	}

	/*
	 * Checks if the C++ string occurs at the beginning of the BufferSegment
	 * in question.
	 */
	bool prefix(const std::string& str) const
	{
		return (prefix((const uint8_t *)str.c_str(), str.length()));
	}
};

/*
 * A Buffer is a list of reference-counted, shareable BufferSegments.
 * Operations on Buffers are translated into the appropriate BufferSegment
 * operation and may result in the modification of, copying of (i.e. COW) and
 * creation of BufferSegments, including both their data and their metadata.
 *
 * For example, appending data from an external source to a Buffer may involve
 * the creation of BufferSegments and copying data from said source into them,
 * or if there is room in the last BufferSegment in the Buffer, the data may be
 * appended to the BufferSegment in place.
 *
 * Almost all operations are built on either BufferSegment operations or
 * operations on the list that contains them, with the exception of a cached
 * length value which is reliable since BufferSegments must not change out from
 * under a Buffer in data or metadata, which provides for much faster
 * performance than scanning the entire list in every possible scenario.
 */
class Buffer {
public:
	typedef	std::deque<BufferSegment *> segment_list_t;

	/*
	 * A SegmentIterator allows for enumeration of the BufferSegments that
	 * comprise a Buffer.
	 */
	class SegmentIterator {
		const segment_list_t& list_;
		segment_list_t::const_iterator iterator_;
	public:
		SegmentIterator(const segment_list_t& list,
				segment_list_t::const_iterator& iterator)
		: list_(list),
		  iterator_(iterator)
		{ }

		SegmentIterator(const segment_list_t& list)
		: list_(list),
		  iterator_(list_.begin())
		{ }

		~SegmentIterator()
		{ }

		const BufferSegment *operator* (void) const
		{
			if (iterator_ == list_.end())
				return (NULL);
			return (*iterator_);
		}

		bool end(void) const
		{
			return (iterator_ == list_.end());
		}

		void next(void)
		{
			if (iterator_ == list_.end())
				return;
			++iterator_;
		}
	};
private:
	size_t length_;
	segment_list_t data_;
public:
	/*
	 * Create a Buffer with no BufferSegments.
	 */
	Buffer(void)
	: length_(0),
	  data_()
	{ }

	/*
	 * Create a Buffer and append byte data to it from an external source.
	 */
	Buffer(const uint8_t *buf, size_t len)
	: length_(0),
	  data_()
	{
		append(buf, len);
	}

	/*
	 * Create a Buffer and append data to it from another Buffer.
	 */
	Buffer(const Buffer& source)
	: length_(0),
	  data_()
	{
		append(source);
	}

	/*
	 * Create a Buffer and append at most len bytes of data to it from
	 * another Buffer.
	 */
	Buffer(const Buffer& source, size_t len)
	: length_(0),
	  data_()
	{
		append(source, len);
	}

	/*
	 * Create a Buffer and append C++ std::string data to it.  This is not
	 * optimal (goes through c_str()), but is also not very common.
	 */
	Buffer(const std::string& str)
	: length_(0),
	  data_()
	{
		append((const uint8_t *)str.c_str(), str.length());
	}

	/*
	 * Merely clear a Buffer out to destruct it.
	 */
	~Buffer()
	{
		clear();
	}

	/*
	 * Overwrite this Buffer's data with that of another buffer.
	 */
	Buffer& operator= (const Buffer& source)
	{
		clear();
		append(&source);
		return (*this);
	}

	/*
	 * Overwrite this Buffer's data with that of a C++ std::string.
	 */
	Buffer& operator= (const std::string& str)
	{
		clear();
		append(str);
		return (*this);
	}

	/*
	 * Append a C++ std::string's data to a Buffer.  This is not optimal
	 * (goes through c_str()), but is also not very common.
	 */
	void append(const std::string& str)
	{
		append((const uint8_t *)str.c_str(), str.length());
	}

	/*
	 * Append BufferSegments from another Buffer to this Buffer.
	 */
	void append(const Buffer& buf)
	{
		segment_list_t::const_iterator it;

		for (it = buf.data_.begin(); it != buf.data_.end(); ++it)
			append(*it);
	}

	/*
	 * Append at most len bytes of data from another Buffer to this Buffer.
	 */
	void append(const Buffer& buf, size_t len)
	{
		if (len == 0)
			return;

		ASSERT(len <= buf.length());
		append(buf);
		if (buf.length() != len)
			trim(buf.length() - len);
	}

	/*
	 * Append BufferSegments from another Buffer to this Buffer.
	 */
	void append(const Buffer *buf)
	{
		append(*buf);
	}

	/*
	 * Append at most len bytes of data from another Buffer to this Buffer.
	 */
	void append(const Buffer *buf, size_t len)
	{
		append(*buf, len);
	}

	/*
	 * Append a single BufferSegment to this Buffer.
	 */
	void append(BufferSegment *seg)
	{
		ASSERT(seg->length() != 0);
		seg->ref();
		data_.push_back(seg);
		length_ += seg->length();
	}

	/*
	 * Append a single byte to this Buffer.
	 */
	void append(uint8_t ch)
	{
		append(&ch, 1);
	}

	/*
	 * Append multiple bytes to this Buffer.
	 */
	void append(const uint8_t *buf, size_t len)
	{
		BufferSegment *seg;
		unsigned o;

		/*
		 * If we are adding less than a single segment worth of data,
		 * try to append it to the end of the last segment.
		 */
		if (len < BUFFER_SEGMENT_SIZE && !data_.empty()) {
			segment_list_t::reverse_iterator it = data_.rbegin();
			seg = *it;
			if (seg->refs() == 1 && seg->avail() >= len) {
				seg = seg->append(buf, len);
				*it = seg;
				length_ += len;
				return;
			}
		}

		/*
		 * Complete segments.
		 */
		for (o = 0; o < len / BUFFER_SEGMENT_SIZE; o++) {
			seg = new BufferSegment();
			seg->append(buf + (o * BUFFER_SEGMENT_SIZE), BUFFER_SEGMENT_SIZE);
			append(seg);
			seg->unref();
		}
		if (len % BUFFER_SEGMENT_SIZE == 0)
			return;
		seg = new BufferSegment();
		seg->append(buf + (o * BUFFER_SEGMENT_SIZE), len % BUFFER_SEGMENT_SIZE);
		append(seg);
		seg->unref();
	}

	/*
	 * Drop references to all BufferSegments in this Buffer and remove them
	 * from its list.
	 */
	void clear(void)
	{
		segment_list_t::iterator it;

		for (it = data_.begin(); it != data_.end(); ++it) {
			BufferSegment *seg = *it;

			length_ -= seg->length();
			seg->unref();
		}
		data_.clear();
		ASSERT(length_ == 0);
	}

	/*
	 * Copy up to dstsize bytes out of this Buffer starting at offset to a
	 * byte buffer.
	 */
	size_t copyout(uint8_t *dst, unsigned offset, size_t dstsize) const
	{
		segment_list_t::const_iterator it;
		size_t copied;

		copied = 0;

		for (it = data_.begin(); it != data_.end(); ++it) {
			const BufferSegment *seg = *it;

			if (offset >= seg->length()) {
				offset -= seg->length();
				continue;
			}

			copied += seg->copyout(dst + copied, offset, dstsize - copied);
			offset = 0;
			if (copied == dstsize)
				break;
		}
		return (copied);
	}

	/*
	 * Copy up to dstsize bytes out of the beginning of this Buffer to a
	 * byte buffer.
	 */
	size_t copyout(uint8_t *dst, size_t dstsize) const
	{
		return (copyout(dst, 0, dstsize));
	}

	/*
	 * Append at most len bytes from the start of this Buffer to a
	 * specified BufferSegment.
	 */
	size_t copyout(BufferSegment *seg, size_t len) const
	{
		ASSERT(seg->avail() >= len);
		len = copyout(seg->tail(), len);
		seg->set_length(seg->length() + len);
		return (len);
	}

	/*
	 * Take a reference to a BufferSegment of len bytes and create one
	 * from the start of this Buffer if the first BufferSegment is not of
	 * the expected length.
	 */
	size_t copyout(BufferSegment **segp, size_t len) const
	{
		BufferSegment *src = data_.front();
		if (src->length() == len) {
			src->ref();
			*segp = src;
			return (len);
		}
		/* XXX Could use trim() if src->length() > len.  */
		BufferSegment *seg = new BufferSegment();
		*segp = seg;
		return (copyout(seg, len));
	}

	/*
	 * Extract an 8-bit quantity out of this Buffer starting at offset.
	 * No endianness is assumed.
	 */
	void extract(uint8_t *p, unsigned offset) const
	{
		size_t copied = copyout(p, offset, sizeof *p);
		ASSERT(copied == sizeof *p);
	}

	/*
	 * Extract an 8-bit quantity out of the beginning of this Buffer.  No
	 * endianness is assumed.
	 */
	void extract(uint8_t *p) const
	{
		extract(p, 0);
	}

	/*
	 * Extract a 16-bit quantity out of this Buffer starting at offset.
	 * No endianness is assumed.
	 */
	void extract(uint16_t *p, unsigned offset) const
	{
		uint8_t data[sizeof *p];
		size_t copied = copyout(data, offset, sizeof data);
		ASSERT(copied == sizeof data);
		memcpy(p, data, sizeof data);
	}

	/*
	 * Extract a 16-bit quantity out of the beginning of this Buffer.  No
	 * endianness is assumed.
	 */
	void extract(uint16_t *p) const
	{
		extract(p, 0);
	}

	/*
	 * Extract a 32-bit quantity out of this Buffer starting at offset.
	 * No endianness is assumed.
	 */
	void extract(uint32_t *p, unsigned offset) const
	{
		uint8_t data[sizeof *p];
		size_t copied = copyout(data, offset, sizeof data);
		ASSERT(copied == sizeof data);
		memcpy(p, data, sizeof data);
	}

	/*
	 * Extract a 32-bit quantity out of the beginning of this Buffer.  No
	 * endianness is assumed.
	 */
	void extract(uint32_t *p) const
	{
		extract(p, 0);
	}

	/*
	 * Extract a 64-bit quantity out of this Buffer starting at offset.
	 * No endianness is assumed.
	 */
	void extract(uint64_t *p, unsigned offset) const
	{
		uint8_t data[sizeof *p];
		size_t copied = copyout(data, offset, sizeof data);
		ASSERT(copied == sizeof data);
		memcpy(p, data, sizeof data);
	}

	/*
	 * Extract a 64-bit quantity out of the beginning of this Buffer.  No
	 * endianness is assumed.
	 */
	void extract(uint64_t *p) const
	{
		extract(p, 0);
	}

	/*
	 * Extract a string in std::string format out of the beginning of this
	 * Buffer.
	 */
	void extract(std::string& str) const
	{
		segment_list_t::const_iterator it;

		for (it = data_.begin(); it != data_.end(); ++it) {
			const BufferSegment *seg = *it;

			str += std::string((const char *)seg->data(), seg->length());
		}
	}

	/*
	 * Get a SegmentIterator that can be used to enumerate BufferSegments
	 * in this Buffer.
	 */
	SegmentIterator segments(void) const
	{
		return (SegmentIterator(data_));
	}

	/*
	 * Returns true if this Buffer is empty.
	 */
	bool empty(void) const
	{
		return (data_.empty());
	}

	/*
	 * Returns true if this Buffer's contents are identical to those of a
	 * specified Buffer.
	 */
	bool equal(const Buffer *buf) const
	{
		if (length() != buf->length())
			return (false);
		return (prefix(buf));
	}

	/*
	 * Returns true if this Buffer's contents are identical to those of a
	 * specified byte-buffer.
	 */
	bool equal(const uint8_t *buf, size_t len) const
	{
		if (len != length())
			return (false);
		return (prefix(buf, len));
	}

	/*
	 * Returns true if this Buffer's contents are identical to those of a
	 * specified C++ std::string.
	 */
	bool equal(const std::string& str) const
	{
		Buffer tmp(str);
		return (equal(&tmp));
	}

	/*
	 * Finds the first occurance of character ch in this Buffer's data and
	 * sets offsetp to the offset it was found at.  If a limit is given, at
	 * most that many characters will be searched.
	 */
	bool find(uint8_t ch, unsigned *offsetp, size_t limit = 0) const
	{
		segment_list_t::const_iterator it;
		unsigned offset;

		offset = 0;

		if (limit == 0)
			limit = length();

		for (it = data_.begin(); it != data_.end(); ++it) {
			const BufferSegment *seg = *it;
			const uint8_t *p;
			size_t len = limit < seg->length() ? limit : seg->length();

			p = (const uint8_t *)memchr(seg->data(), ch, len);
			if (p == NULL) {
				offset += len;
				limit -= len;

				if (limit == 0)
					break;
				continue;
			}

			*offsetp = offset + (p - seg->data());
			return (true);
		}
		return (false);
	}

	/*
	 * Returns the current amount of data associated with this Buffer.
	 */
	size_t length(void) const
	{
		return (length_);
	}

	/*
	 * Move up to dstsize bytes out of this Buffer starting at offset and
	 * into a supplied byte-buffer.
	 */
	void moveout(uint8_t *dst, unsigned offset, size_t dstsize)
	{
		ASSERT(length() >= offset + dstsize);
		size_t copied = copyout(dst, offset, dstsize);
		ASSERT(copied == dstsize);
		skip(offset + dstsize);
	}

	/*
	 * Move up to dstsize bytes from the start of this Buffer and into a
	 * supplied byte-buffer.
	 */
	void moveout(uint8_t *dst, size_t dstsize)
	{
		moveout(dst, 0, dstsize);
	}

	/*
	 * Move up to dstsize bytes out of this Buffer starting at offset and
	 * into a supplied Buffer.
	 */
	void moveout(Buffer *dst, unsigned offset, size_t dstsize)
	{
		ASSERT(length() >= offset + dstsize);
		if (offset != 0)
			skip(offset);
		dst->append(this, dstsize);
		skip(dstsize);
	}

	/*
	 * Move up to dstsize bytes from the start of this Buffer and into a
	 * supplied Buffer.
	 */
	void moveout(Buffer *dst, size_t dstsize)
	{
		moveout(dst, 0, dstsize);
	}

	/*
	 * Move the first BufferSegment in this Buffer out of it and give our
	 * reference to it to the caller.
	 */
	void moveout(BufferSegment **segp)
	{
		ASSERT(!empty());
		BufferSegment *seg = data_.front();
		data_.pop_front();
		length_ -= seg->length();
		*segp = seg;
	}

	/*
	 * Look at the first character in this Buffer.
	 */
	uint8_t peek(void) const
	{
		ASSERT(length_ != 0);

		const BufferSegment *seg = data_.front();
		return (seg->data()[0]);
	}

	/*
	 * Returns true if the supplied C++ std::string is a prefix of this
	 * Buffer.
	 */
	bool prefix(const std::string& str) const
	{
		ASSERT(str.length() > 0);
		if (str.length() > length())
			return (false);
		Buffer tmp(str);
		return (prefix(&tmp));
	}

	/*
	 * Returns true if the supplied byte-buffer is a prefix of this Buffer.
	 */
	bool prefix(const uint8_t *buf, size_t len) const
	{
		ASSERT(len > 0);
		if (len > length())
			return (false);
		Buffer tmp(buf, len);
		return (prefix(&tmp));
	}

	/*
	 * Returns true if the supplied Buffer's data is a prefix of this
	 * Buffer.
	 */
	bool prefix(const Buffer *buf) const
	{
		ASSERT(buf->length() > 0);
		if (buf->length() > length())
			return (false);

		segment_list_t::const_iterator pfx = buf->data_.begin();

		segment_list_t::const_iterator big = data_.begin();
		const BufferSegment *bigseg = *big;
		const uint8_t *q = bigseg->data();

		while (pfx != buf->data_.end()) {
			const BufferSegment *pfxseg = *pfx;
			const uint8_t *p = pfxseg->data();
			while (p < pfxseg->end()) {
				if (q >= bigseg->end()) {
					big++;
					bigseg = *big;
					q = bigseg->data();
				}
				if (*q++ == *p++)
					continue;
				return (false);
			}
			pfx++;
		}
		return (true);
	}

	/*
	 * Remove the leading bytes from this Buffer.
	 *
	 * XXX Keep in sync with trim().
	 */
	void skip(size_t bytes)
	{
		segment_list_t::iterator it;
		unsigned skipped;

		ASSERT(!empty());

		if (bytes == length()) {
			clear();
			return;
		}

		skipped = 0;

		while ((it = data_.begin()) != data_.end()) {
			BufferSegment *seg = *it;

			if (skipped == bytes)
				break;

			/*
			 * Skip entire segments.
			 */
			if ((bytes - skipped) >= seg->length()) {
				skipped += seg->length();
				data_.erase(it);
				seg->unref();
				continue;
			}

			/*
			 * Skip a partial segment.
			 */
			seg = seg->skip(bytes - skipped);
			*it = seg;
			skipped += bytes - skipped;
			ASSERT(skipped == bytes);
			break;
		}
		length_ -= skipped;
	}

	/*
	 * Remove the trailing bytes from this Buffer.
	 *
	 * XXX Keep in sync with skip().
	 */
	void trim(size_t bytes)
	{
		segment_list_t::iterator it;
		unsigned trimmed;

		ASSERT(!empty());

		if (bytes == length()) {
			clear();
			return;
		}

		trimmed = 0;

		while ((it = --data_.end()) != data_.end()) {
			BufferSegment *seg = *it;

			if (trimmed == bytes)
				break;

			/*
			 * Trim entire segments.
			 */
			if ((bytes - trimmed) >= seg->length()) {
				trimmed += seg->length();
				data_.erase(it);
				seg->unref();
				continue;
			}

			/*
			 * Trim a partial segment.
			 */
			seg = seg->trim(bytes - trimmed);
			*it = seg;
			trimmed += bytes - trimmed;
			ASSERT(trimmed == bytes);
			break;
		}
		length_ -= trimmed;
	}

	/*
	 * Remove internal bytes from this Buffer.
	 *
	 * NB: skip() and trim() could be implemented in terms of cut(), but
	 * it is a marked pessimization to do so.  It would, however, make
	 * sense to merge skip and trim and to just pick the iterator based on
	 * the end it's being done at.
	 */
	void cut(unsigned offset, size_t bytes)
	{
		segment_list_t::iterator it;

		ASSERT(offset <= length_);
		ASSERT(bytes != 0);
		ASSERT(offset + bytes <= length_);

		/* Remove from start.  */
		if (offset == 0) {
			skip(bytes);
			return;
		}

		/* Remove from end.  */
		if (offset + bytes == length_) {
			trim(bytes);
			return;
		}

		/* Preemptively adjust length.  */
		length_ -= bytes;

		it = data_.begin();
		while (it != data_.end()) {
			BufferSegment *seg = *it;

			ASSERT(bytes != 0);

			if (offset >= seg->length()) {
				++it;
				offset -= seg->length();
				continue;
			}

			/* Remove this element, point iterator at the next one.  */
			it = data_.erase(it);
			if (offset + bytes >= seg->length()) {
				if (offset == 0) {
					/* We do not need this segment at all.  */
					bytes -= seg->length();
					seg->unref();
					offset = 0;

					if (bytes == 0)
						break;
					continue;
				}
				/* We need only the first offset bytes of this segment.  */
				bytes -= seg->length() - offset;
				seg = seg->truncate(offset);
				ASSERT(seg->length() == offset);
				offset = 0;

				data_.insert(it, seg);

				if (bytes == 0)
					break;
				continue;
			}

			/* This is the final segment.  */
			if (offset != 0) {
				/* Get any leading data.  */
				seg->ref();
				data_.insert(it, seg->truncate(offset));
				offset = 0;
			}

			seg = seg->skip(offset + bytes);
			data_.insert(it, seg);

			bytes -= bytes;
			break;
		}
		ASSERT(offset == 0);
		ASSERT(bytes == 0);
	}

	/*
	 * Truncate this Buffer to the specified length.
	 */
	void truncate(size_t len)
	{
		if (length_ == len)
			return;
		ASSERT(length_ > len);
		trim(length_ - len);
	}

	/*
	 * Apply to a function each BufferSegment in this Buffer.
	 */
	template<typename T>
	void foreach_segment(T& f) const
	{
		segment_list_t::const_iterator it;

		for (it = data_.begin(); it != data_.end(); ++it)
			f(*it);
	}

	/*
	 * Fill a suppled iovec which has at most a specified number of elements
	 * with the contents of this Buffer and return the number which were
	 * populated.
	 */
	size_t fill_iovec(struct iovec *, size_t) const;

	/*
	 * Create a string with a canonical hexdump of the contents of this
	 * Buffer.
	 */
	std::string hexdump(unsigned = 0) const;
};

std::ostream& operator<< (std::ostream&, const Buffer *);
std::ostream& operator<< (std::ostream&, const Buffer&);

#endif /* !BUFFER_H */
