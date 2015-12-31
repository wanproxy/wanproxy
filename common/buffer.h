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

#ifndef	COMMON_BUFFER_H
#define	COMMON_BUFFER_H

#include <string.h> /* memmove(3), memcpy(3), etc.  */

#include <deque>
#include <vector>

#include <common/refcount.h>

/*
 * MT NB:
 *
 * So, a Buffer cannot be shared between threads without higher-level
 * synchronization, but a BufferSegment can.
 *
 * Atomic reference counting should be sufficient.  The only case where it
 * could have shortcomings even in theory would be transitions from a count
 * of 1, at which point only the running thread should have access to the
 * BufferSegment pointer unless a Buffer is being shared between threads
 * without synchronization which itself would be violently-broken.  So some
 * kind of atomics and reference counting should suffice.
 */

struct iovec;

#define	BUFFER_SEGMENT_SIZE		(2048)

typedef	unsigned buffer_segment_size_t;

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
 *
 * A BufferSegment may also refer to an external chunk of data, either data
 * in another BufferSegment, or in some other data structure.  This allows
 * BufferSegment and Buffer users to avoid copies in many cases, both where
 * there is data coming from an external source, and where metadata is being
 * manipulated, but data is constant and shared.
 */
class BufferSegment {
	typedef	void data_free_t(void *, uint8_t *, buffer_segment_size_t, buffer_segment_size_t);

	uint8_t *data_;
	buffer_segment_size_t offset_;
	buffer_segment_size_t length_;
	RefCount ref_;
	data_free_t *data_free_;
	void *data_free_arg_;

	/*
	 * Creates a new, empty BufferSegment with a single reference.
	 */
	BufferSegment(void)
	: data_(NULL),
	  offset_(0),
	  length_(0),
	  ref_(),
	  data_free_(NULL),
	  data_free_arg_(NULL)
	{
		/* XXX Built-in slab allocator?  */
		data_ = (uint8_t *)malloc(BUFFER_SEGMENT_SIZE);
	}

	/*
	 * Creates a new BufferSegment with independent metadata and
	 * external/shared data.
	 */
	BufferSegment(uint8_t *xdata, buffer_segment_size_t offset, buffer_segment_size_t xlength, data_free_t *data_free, void *data_free_arg)
	: data_(xdata),
	  offset_(offset),
	  length_(xlength),
	  ref_(),
	  data_free_(data_free),
	  data_free_arg_(data_free_arg)
	{
		ASSERT_NON_NULL("/buffer/segment", data_);
		ASSERT("/buffer/segment", offset_ <= BUFFER_SEGMENT_SIZE);
		ASSERT("/buffer/segment", length_ <= BUFFER_SEGMENT_SIZE);
		ASSERT("/buffer/segment", offset_ + length_ <= BUFFER_SEGMENT_SIZE);
		ASSERT("/buffer/segment", length_ > 0);
		ASSERT_NON_NULL("/buffer/segment", data_free_);
	}

	/*
	 * Should almost always only be called from unref().
	 */
	~BufferSegment()
	{
		if (data_ != NULL) {
			if (data_free_ == NULL)
				free(data_);
			else
				data_free_(data_free_arg_, data_, offset_, length_);
			data_ = NULL;
		}
	}

public:
	/*
	 * Get an empty BufferSegment.
	 */
	static BufferSegment *create(void)
	{
		return (new BufferSegment());
	}

	/*
	 * Get a BufferSegment with the requested data.
	 */
	static BufferSegment *create(const uint8_t *buf, size_t len)
	{
		ASSERT_NON_NULL("/buffer/segment", buf);
		ASSERT_NON_ZERO("/buffer/segment", len);
		ASSERT("/buffer/segment", len <= BUFFER_SEGMENT_SIZE);

		BufferSegment *seg = create();
		memcpy(seg->data_, buf, len);
		seg->length_ = len;

		return (seg);
	}

	/*
	 * Bump the reference count.
	 */
	void ref(void)
	{
		ref_.hold();
	}

	/*
	 * Drop the reference count and delete if there are no other live
	 * references.
	 */
	void unref(void)
	{
		if (ref_.drop())
			delete this;
	}

	/*
	 * Returns true if the BufferSegment's data is held exclusively.
	 */
	bool data_exclusive(void) const
	{
		return (ref_.exclusive() && data_free_ == NULL);
	}

	/*
	 * Returns true if the BufferSegment's metadata is held exclusively.
	 */
	bool metadata_exclusive(void) const
	{
		return (ref_.exclusive());
	}

	/*
	 * Return a mutable pointer to the start of the data.  Discouraaged.
	 */
	uint8_t *head(void)
	{
		ASSERT("/buffer/segment", data_exclusive());
		return (&data_[offset_]);
	}

	/*
	 * Return a mutable pointer to the point at which data may be
	 * appended.  Discouraged.
	 */
	uint8_t *tail(void)
	{
		ASSERT("/buffer/segment", data_exclusive());
		return (&data_[offset_ + length_]);
	}

	/*
	 * Append data.
	 */
	BufferSegment *append(const uint8_t *buf, size_t len)
	{
		ASSERT_NON_NULL("/buffer/segment", buf);
		ASSERT_NON_ZERO("/buffer/segment", len);
		ASSERT("/buffer/segment", len <= avail());
		if (!data_exclusive()) {
			BufferSegment *seg;

			seg = this->unshare();
			if (seg != this) {
				seg = seg->append(buf, len);
				return (seg);
			}
		}
		if (avail() - offset_ < len)
			pullup();
		memcpy(tail(), buf, len);
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
		ASSERT("/buffer/segment", !str.empty());
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
	 * Clone a BufferSegment.
	 */
	BufferSegment *copy(void) const
	{
		ASSERT_NON_ZERO("/buffer/segment", length_);
		BufferSegment *seg = BufferSegment::create(data(), length_);
		return (seg);
	}

	/*
	 * Clone a BufferSegment, but share its data.
	 * Passes the caller's reference to the clone.
	 */
	BufferSegment *metacopy(void)
	{
		ASSERT_NON_ZERO("/buffer/segment", length_);
		return (new BufferSegment(data_, offset_, length_, &BufferSegment::free_meta, this));
	}

	/*
	 * We want an unshared copy of this BufferSegment.
	 *
	 * Consumes the caller's reference to this and returns with a
	 * reference to the new BufferSegment.  The new BufferSegment
	 * may be the same as this BufferSegment if we have exclusive
	 * metadata already and just need our own copy of data.
	 */
	BufferSegment *unshare(void)
	{
		if (!metadata_exclusive()) {
			BufferSegment *seg;

			seg = this->copy();
			this->unref();
			return (seg);
		}

		/*
		 * Drop our interest in the external data.
		 *
		 * We may also get here without external
		 * data in the case of a race.  The caller
		 * checks whether it has exclusive
		 * ownership of data, which is never true
		 * for external buffers.  We then check if
		 * we have exclusive ownership of the
		 * metadata.  If another thread dropped its
		 * claim to the metadata between the first
		 * check and the second, then we would
		 * expect to end up here, with exclusive
		 * metadata, and actually with exclusive
		 * data as well.
		 */
		if (data_free_ != NULL) {
			uint8_t *data;

			data = (uint8_t *)malloc(BUFFER_SEGMENT_SIZE);
			copyout(data, 0, length_);

			ASSERT_NON_NULL("/buffer/segment", data_free_);
			data_free_(data_free_arg_, data_, offset_, length_);
			data_free_ = NULL;

			data_ = data;
			offset_ = 0;
		} else {
			/*
			 * Nothing do do; we have won a race in our
			 * favour, and now have an unshared copy of
			 * the data.
			 */
		}
		return (this);
	}

	/*
	 * Copy out the requested number of bytes or the entire available
	 * length, whichever is smaller.  Returns the amount of data read.
	 */
	void copyout(uint8_t *dst, unsigned offset, size_t dstsize) const
	{
		ASSERT_NON_NULL("/buffer/segment", dst);
		ASSERT_NON_ZERO("/buffer/segment", dstsize);
		ASSERT("/buffer/segment", length() >= offset + dstsize);

		memcpy(dst, data() + offset, dstsize);
	}

	/*
	 * Return a read-only pointer to the start of data in the segment.
	 */
	const uint8_t *data(void) const
	{
		ASSERT_NON_ZERO("/buffer/segment", length_);
		return (&data_[offset_]);
	}

	/*
	 * Return a read-only pointer to the character after the end of the
	 * data.  If writable, this would be the point at which new data
	 * would be appended.
	 */
	const uint8_t *end(void) const
	{
		ASSERT_NON_ZERO("/buffer/segment", length_);
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
		ASSERT_NON_ZERO("/buffer/segment", length_);
		ASSERT("/buffer/segment", data_exclusive());
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
		ASSERT("/buffer/segment", data_exclusive());
		ASSERT_ZERO("/buffer/segment", offset_);
		ASSERT("/buffer/segment", len <= BUFFER_SEGMENT_SIZE);
		length_ = len;
	}

	/*
	 * Skip a number of bytes at the start of a BufferSemgent.  Creates a
	 * copy if there are other live references.
	 */
	BufferSegment *skip(unsigned bytes)
	{
		ASSERT_NON_ZERO("/buffer/segment", bytes);
		ASSERT("/buffer/segment", bytes < length());

		if (!metadata_exclusive())
			return (this->metacopy()->skip(bytes));
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
		ASSERT_NON_ZERO("/buffer/segment", bytes);
		ASSERT("/buffer/segment", bytes < length());

		if (!metadata_exclusive())
			return (this->metacopy()->trim(bytes));
		length_ -= bytes;

		return (this);
	}

	/*
	 * Remove bytes at offset in the BufferSegment.  Creates a copy if
	 * there are live references.
	 */
	BufferSegment *cut(unsigned offset, unsigned bytes)
	{
		ASSERT_NON_ZERO("/buffer/segment", bytes);
		ASSERT("/buffer/segment", offset + bytes < length());

		if (offset == 0)
			return (this->skip(bytes));

		if (offset + bytes == length())
			return (this->trim(bytes));

		if (!data_exclusive()) {
			BufferSegment *seg;

			seg = BufferSegment::create(this->data(), offset);
			seg->append(this->data() + offset + bytes,
				    this->length() - (offset + bytes));
			this->unref();
			return (seg);
		}

		memmove(head() + offset, head() + offset + bytes,
			length() - (offset + bytes));
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
	bool equal(const uint8_t *buf, size_t len) const
	{
		ASSERT_NON_ZERO("/buffer/segment", len);
		if (len != length())
			return (false);
		return (memcmp(data(), buf, len) == 0);
	}

	/*
	 * Checks if the BufferSegment's contents are identical to the given
	 * C++ string.
	 */
	bool equal(const std::string& str) const
	{
		if (str.empty())
			return (length() == 0);
		return (equal((const uint8_t *)str.c_str(), str.length()));
	}

	/*
	 * Checks if the contents of two BufferSegments are identical.
	 */
	bool equal(const BufferSegment *seg) const
	{
		return (equal(seg->data(), seg->length()));
	}

	static void free_meta(void *arg, uint8_t *, buffer_segment_size_t, buffer_segment_size_t)
	{
		BufferSegment *seg = (BufferSegment *)arg;
		seg->unref();
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
		ASSERT_NON_NULL("/buffer", buf);
		ASSERT_NON_ZERO("/buffer", len);
		append(buf, len);
	}

	/*
	 * Create a Buffer and append data to it from another Buffer.
	 */
	Buffer(const Buffer& source)
	: length_(0),
	  data_()
	{
		if (!source.empty())
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
		if (!str.empty())
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
		if (!str.empty())
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
		ASSERT_NON_ZERO("/buffer", len);
		ASSERT("/buffer", len <= buf.length());
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
		ASSERT("/buffer", seg->length() != 0);
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
	 * Prevent downcasts to uint8_t.
	 */
private:
	void append(uint16_t);
	void append(uint32_t);
	void append(uint64_t);
public:

	/*
	 * Append multiple bytes to this Buffer.
	 */
	void append(const uint8_t *buf, size_t len)
	{
		BufferSegment *seg;
		unsigned o;

		ASSERT_NON_ZERO("/buffer", len);

		/*
		 * If we are adding less than a single segment worth of data,
		 * try to append it to the end of the last segment.
		 */
		if (len < BUFFER_SEGMENT_SIZE && !data_.empty()) {
			segment_list_t::reverse_iterator it = data_.rbegin();
			seg = *it;
			if (seg->data_exclusive() && seg->avail() >= len) {
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
			seg = BufferSegment::create(buf + (o * BUFFER_SEGMENT_SIZE), BUFFER_SEGMENT_SIZE);
			append(seg);
			seg->unref();
		}
		if (len % BUFFER_SEGMENT_SIZE == 0)
			return;
		seg = BufferSegment::create(buf + (o * BUFFER_SEGMENT_SIZE), len % BUFFER_SEGMENT_SIZE);
		append(seg);
		seg->unref();
	}

	/*
	 * Append a 16-bit quantity to this buffer.  We always use a
	 * pointer here unlike with append(uint8_t) so that differences in
	 * type mus be explicit.  Also, we don't have a variant like this
	 * for uint8_t since on platforms where 'char' is unsigned, a
	 * literal could end up here rather than the std::string overload.
	 */
	void append(const uint16_t *p)
	{
		uint8_t data[sizeof *p];
		memcpy(data, p, sizeof data);
		append(data, sizeof data);
	}

	/*
	 * Append a 32-bit quantity to this buffer.
	 *
	 * See append(const uint16_t *) for more information.
	 */
	void append(const uint32_t *p)
	{
		uint8_t data[sizeof *p];
		memcpy(data, p, sizeof data);
		append(data, sizeof data);
	}

	/*
	 * Append a 64-bit quantity to this buffer.
	 *
	 * See append(const uint16_t *) for more information.
	 */
	void append(const uint64_t *p)
	{
		uint8_t data[sizeof *p];
		memcpy(data, p, sizeof data);
		append(data, sizeof data);
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

			ASSERT("/buffer", length_ >= seg->length());
			length_ -= seg->length();
			seg->unref();
		}
		data_.clear();
		ASSERT_ZERO("/buffer", length_);
	}

	/*
	 * Copy up to dstsize bytes out of this Buffer starting at offset to a
	 * byte buffer.
	 */
	void copyout(uint8_t *dst, size_t offset, size_t dstsize) const
	{
		segment_list_t::const_iterator it;

		ASSERT("/buffer", offset + dstsize <= length_);

		for (it = data_.begin(); it != data_.end(); ++it) {
			const BufferSegment *seg = *it;

			if (offset >= seg->length()) {
				offset -= seg->length();
				continue;
			}

			size_t seglen = seg->length() - offset;
			if (dstsize > seglen) {
				seg->copyout(dst, offset, seglen);
				dst += seglen;
				dstsize -= seglen;
				offset = 0;
				continue;
			}
			seg->copyout(dst, offset, dstsize);
			break;
		}
	}

	/*
	 * Copy up to dstsize bytes out of the beginning of this Buffer to a
	 * byte buffer.
	 */
	void copyout(uint8_t *dst, size_t dstsize) const
	{
		return (copyout(dst, 0, dstsize));
	}

	/*
	 * Take a reference to the BufferSegment at the start of this Buffer.
	 */
	void copyout(BufferSegment **segp) const
	{
		ASSERT("/buffer", !empty());
		BufferSegment *src = data_.front();
		src->ref();
		*segp = src;
	}

	/*
	 * Take a reference to a BufferSegment of len bytes and create one
	 * from the start of this Buffer if the first BufferSegment is not of
	 * the expected length.
	 */
	void copyout(BufferSegment **segp, size_t len) const
	{
		ASSERT_NON_ZERO("/buffer", len);
		ASSERT("/buffer", len <= BUFFER_SEGMENT_SIZE);
		ASSERT("/buffer", length() >= len);
		BufferSegment *src = data_.front();
		if (src->length() == len) {
			src->ref();
			*segp = src;
			return;
		}
		if (src->length() > len) {
			src->ref();
			*segp = src->truncate(len);
			return;
		}
		BufferSegment *seg = src->copy();
		copyout(seg->tail(), seg->length(), len - seg->length());
		seg->set_length(len);
		*segp = seg;
	}

	/*
	 * Extract an 8-bit quantity out of this Buffer starting at offset.
	 * No endianness is assumed.
	 */
	void extract(uint8_t *p, size_t offset = 0) const
	{
		ASSERT("/buffer", length() >= offset + sizeof *p);
		copyout(p, offset, sizeof *p);
	}

	/*
	 * Extract a 16-bit quantity out of this Buffer starting at offset.
	 * No endianness is assumed.
	 */
	void extract(uint16_t *p, size_t offset = 0) const
	{
		ASSERT("/buffer", length() >= offset + sizeof *p);
		copyout((uint8_t *)p, offset, sizeof *p);
	}

	/*
	 * Extract a 32-bit quantity out of this Buffer starting at offset.
	 * No endianness is assumed.
	 */
	void extract(uint32_t *p, size_t offset = 0) const
	{
		ASSERT("/buffer", length() >= offset + sizeof *p);
		copyout((uint8_t *)p, offset, sizeof *p);
	}

	/*
	 * Extract a 64-bit quantity out of this Buffer starting at offset.
	 * No endianness is assumed.
	 */
	void extract(uint64_t *p, size_t offset = 0) const
	{
		ASSERT("/buffer", length() >= offset + sizeof *p);
		copyout((uint8_t *)p, offset, sizeof *p);
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
	 * Move a 16-bit quantity out of this Buffer starting at offset.
	 * No endianness is assumed.
	 */
	void moveout(uint16_t *p, size_t offset = 0)
	{
		ASSERT("/buffer", length() >= offset + sizeof *p);
		moveout((uint8_t *)p, offset, sizeof *p);
	}

	/*
	 * Move a 32-bit quantity out of this Buffer starting at offset.
	 * No endianness is assumed.
	 */
	void moveout(uint32_t *p, size_t offset = 0)
	{
		ASSERT("/buffer", length() >= offset + sizeof *p);
		moveout((uint8_t *)p, offset, sizeof *p);
	}

	/*
	 * Move a 64-bit quantity out of this Buffer starting at offset.
	 * No endianness is assumed.
	 */
	void moveout(uint64_t *p, size_t offset = 0)
	{
		ASSERT("/buffer", length() >= offset + sizeof *p);
		moveout((uint8_t *)p, offset, sizeof *p);
	}

	/*
	 * Move a string in std::string format out of the beginning of this
	 * Buffer.
	 */
	void moveout(std::string& str)
	{
		segment_list_t::const_iterator it;

		for (it = data_.begin(); it != data_.end(); ++it) {
			BufferSegment *seg = *it;

			str += std::string((const char *)seg->data(), seg->length());
			seg->unref();
		}
		data_.clear();
		length_ = 0;
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
		if (empty())
			return (true);
		return (prefix(buf));
	}

	/*
	 * Returns true if this Buffer's contents are identical to those of a
	 * specified byte-buffer.
	 */
	bool equal(const uint8_t *buf, size_t len) const
	{
		ASSERT_NON_ZERO("/buffer", len);
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
		if (str.length() != length())
			return (false);
		if (empty())
			return (true);
		return (prefix(str));
	}

	/*
	 * Finds the first occurance of character ch in this Buffer's data and
	 * sets offsetp to the offset it was found at.  If a limit is given, at
	 * most that many characters will be searched.
	 */
	bool find(uint8_t ch, size_t *offsetp, size_t limit = 0) const
	{
		segment_list_t::const_iterator it;
		size_t offset;

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
	 * Finds the first occurance of any character in s in this Buffer's
	 * data and sets offsetp to the offset it was found at.  It indicates
	 * via the foundp parameter which of the set was found.
	 */
	bool find_any(const std::string& s, size_t *offsetp, uint8_t *foundp = NULL) const
	{
		uint8_t set[256];
		segment_list_t::const_iterator it;
		size_t offset;
		size_t sit;

		memset(set, 0, sizeof set);
		for (sit = 0; sit < s.length(); sit++)
			set[(unsigned char)s[sit]] = 1;

		offset = 0;

		for (it = data_.begin(); it != data_.end(); ++it) {
			const BufferSegment *seg = *it;
			const uint8_t *p = seg->data();
			size_t len = seg->length();
			unsigned i;

			for (i = 0; i < len; i++) {
				if (set[p[i]] == 0)
					continue;
				if (foundp != NULL)
					*foundp = p[i];
				*offsetp = offset + i + (p - seg->data());
				return (true);
			}
			offset += len;
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
	void moveout(uint8_t *dst, size_t offset, size_t dstsize)
	{
		ASSERT_NON_ZERO("/buffer", dstsize);
		ASSERT("/buffer", length() >= offset + dstsize);
		copyout(dst, offset, dstsize);
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
	void moveout(Buffer *dst, size_t offset, size_t dstsize)
	{
		ASSERT_NON_ZERO("/buffer", dstsize);
		ASSERT("/buffer", length() >= offset + dstsize);
		if (offset != 0)
			skip(offset);
		skip(dstsize, dst);
	}

	/*
	 * Move up to dstsize bytes from the start of this Buffer and into a
	 * supplied Buffer.
	 */
	void moveout(Buffer *dst, size_t dstsize)
	{
		ASSERT_NON_ZERO("/buffer", dstsize);
		ASSERT("/buffer", length() >= dstsize);
		if (dstsize == length()) {
			moveout(dst);
			return;
		}
		moveout(dst, 0, dstsize);
	}

	/*
	 * Move everything from this Buffer and into a suppled Buffer.
	 */
	void moveout(Buffer *dst)
	{
		if (dst->empty()) {
			dst->data_ = data_;
			data_.clear();

			dst->length_ = length_;
			length_ = 0;

			return;
		}

		dst->data_.insert(dst->data_.end(), data_.begin(), data_.end());
		data_.clear();

		dst->length_ += length_;
		length_ = 0;
	}

	/*
	 * Move the first BufferSegment in this Buffer out of it and give our
	 * reference to it to the caller.
	 */
	void moveout(BufferSegment **segp)
	{
		ASSERT("/buffer", !empty());
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
		ASSERT_NON_ZERO("/buffer", length_);

		const BufferSegment *seg = data_.front();
		return (seg->data()[0]);
	}

	/*
	 * Take the first character in this Buffer.
	 */
	uint8_t pop(void)
	{
		ASSERT_NON_ZERO("/buffer", length_);

		segment_list_t::iterator front = data_.begin();
		BufferSegment *seg = *front;
		uint8_t ch = seg->data()[0];
		if (seg->length() == 1) {
			seg->unref();
			data_.pop_front();
		} else {
			seg = seg->skip(1);
			*front = seg;
		}
		length_--;
		return (ch);
	}

	/*
	 * Returns true if the supplied C++ std::string is a prefix of this
	 * Buffer.
	 */
	bool prefix(const std::string& str) const
	{
		ASSERT("/buffer", str.length() > 0);
		if (str.length() > length())
			return (false);
		return (prefix((const uint8_t *)str.c_str(), str.length()));
	}

	/*
	 * Returns true if the supplied byte-buffer is a prefix of this Buffer.
	 */
	bool prefix(const uint8_t *buf, size_t len) const
	{
		ASSERT("/buffer", len > 0);
		if (len > length())
			return (false);

		const uint8_t *p = buf;
		size_t plen = len;

		segment_list_t::const_iterator big = data_.begin();
		const BufferSegment *bigseg = *big;
		const uint8_t *q = bigseg->data();
		const uint8_t *qe = bigseg->end();
		size_t qlen = qe - q;

		ASSERT_NON_ZERO("/buffer", qlen);

		for (;;) {
			ASSERT_NON_ZERO("/buffer", plen);
			ASSERT_NON_ZERO("/buffer", qlen);

			size_t cmplen = std::min(plen, qlen);
			if (memcmp(q, p, cmplen) != 0)
				return (false);
			plen -= cmplen;
			qlen -= cmplen;

			if (qlen == 0) {
				big++;
				if (big == data_.end())
					break;
				bigseg = *big;
				q = bigseg->data();
				qe = bigseg->end();
				qlen = qe - q;
			} else {
				q += cmplen;
			}

			if (plen == 0)
				break;

			p += cmplen;
		}
		return (true);
	}

	/*
	 * Returns true if the supplied Buffer's data is a prefix of this
	 * Buffer.
	 */
	bool prefix(const Buffer *buf) const
	{
		ASSERT("/buffer", buf->length() > 0);
		if (buf->length() > length())
			return (false);

		segment_list_t::const_iterator pfx = buf->data_.begin();

		segment_list_t::const_iterator big = data_.begin();
		const BufferSegment *bigseg = *big;
		const uint8_t *q = bigseg->data();
		const uint8_t *qe = bigseg->end();
		size_t qlen = qe - q;

		while (pfx != buf->data_.end()) {
			const BufferSegment *pfxseg = *pfx;
			const uint8_t *p = pfxseg->data();
			const uint8_t *pe = pfxseg->end();
			size_t plen = pe - p;

			ASSERT_NON_ZERO("/buffer", qlen);
			ASSERT_NON_ZERO("/buffer", plen);

			for (;;) {
				ASSERT_NON_ZERO("/buffer", plen);
				ASSERT_NON_ZERO("/buffer", qlen);

				size_t cmplen = std::min(plen, qlen);
				if (memcmp(q, p, cmplen) != 0)
					return (false);
				plen -= cmplen;
				qlen -= cmplen;

				if (qlen == 0) {
					big++;
					if (big == data_.end())
						break;
					bigseg = *big;
					q = bigseg->data();
					qe = bigseg->end();
					qlen = qe - q;
				} else {
					q += cmplen;
				}

				if (plen == 0)
					break;

				p += cmplen;
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
	void skip(size_t bytes, Buffer *clip = NULL)
	{
		segment_list_t::iterator it;
		size_t skipped;

		ASSERT_NON_ZERO("/buffer", bytes);
		ASSERT("/buffer", !empty());

		if (bytes == length()) {
			if (clip != NULL) {
				moveout(clip);
				return;
			}
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
				if (clip != NULL)
					clip->append(seg);
				seg->unref();
				continue;
			}

			/*
			 * Skip a partial segment.
			 */
			if (clip != NULL)
				clip->append(seg->data(), bytes - skipped);
			seg = seg->skip(bytes - skipped);
			*it = seg;
			skipped += bytes - skipped;
			ASSERT("/buffer", skipped == bytes);
			break;
		}
		length_ -= skipped;
	}

	/*
	 * Remove the trailing bytes from this Buffer.
	 *
	 * XXX Keep in sync with skip().
	 */
	void trim(size_t bytes, Buffer *clip = NULL)
	{
		segment_list_t::iterator it;
		size_t trimmed;

		ASSERT_NON_ZERO("/buffer", bytes);
		ASSERT("/buffer", !empty());

		if (bytes == length()) {
			if (clip != NULL) {
				moveout(clip);
				return;
			}
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
				if (clip != NULL)
					clip->append(seg);
				seg->unref();
				continue;
			}

			/*
			 * Trim a partial segment.
			 */
			if (clip != NULL)
				clip->append(seg->data() + (seg->length() - (bytes - trimmed)), bytes - trimmed);
			seg = seg->trim(bytes - trimmed);
			*it = seg;
			trimmed += bytes - trimmed;
			ASSERT("/buffer", trimmed == bytes);
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
	void cut(size_t offset, size_t bytes, Buffer *clip = NULL)
	{
		segment_list_t::iterator it;

		ASSERT("/buffer", offset <= length_);
		ASSERT_NON_ZERO("/buffer", bytes);
		ASSERT("/buffer", offset + bytes <= length_);

		/* Remove from start.  */
		if (offset == 0) {
			skip(bytes, clip);
			return;
		}

		/* Remove from end.  */
		if (offset + bytes == length_) {
			trim(bytes, clip);
			return;
		}

		/* Preemptively adjust length.  */
		length_ -= bytes;

		it = data_.begin();
		while (it != data_.end()) {
			BufferSegment *seg = *it;

			ASSERT_NON_ZERO("/buffer", bytes);

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
					if (clip != NULL)
						clip->append(seg);
					seg->unref();

					if (bytes == 0)
						break;
					continue;
				}
				if (clip != NULL)
					clip->append(seg->data() + offset, seg->length() - offset);
				/* We need only the first offset bytes of this segment.  */
				bytes -= seg->length() - offset;
				seg = seg->truncate(offset);
				ASSERT("/buffer", seg->length() == offset);
				offset = 0;

				data_.insert(it, seg);

				if (bytes == 0)
					break;
				continue;
			}

			/*
			 * This is the final segment.
			 *
			 * We could split this into two segments instead of
			 * using BufferSegment::cut().  That would mean doing
			 * (effectively):
			 *     seg->ref();
			 *     seg0 = seg->truncate(offset);
			 *     seg1 = seg->skip(offset + bytes);
			 * If the original seg has only one ref, we can do the
			 * skip in-place on it without modifying data, but we
			 * have to copy it for the truncate.  So we have to
			 * do a copy and now we (inefficiently) have two
			 * segments.
			 *
			 * If we instead use BufferSegment::cut(), we have to
			 * move data around a little if there's only one ref
			 * and do a copy if there's more than one ref.  So we
			 * have probably less overhead and demonstrably
			 * fewer segments in this Buffer.
			 */
			if (clip != NULL)
				clip->append(seg->data() + offset, bytes);
			seg = seg->cut(offset, bytes);
			data_.insert(it, seg);

			offset = 0;

			bytes -= bytes;
			break;
		}
		ASSERT_ZERO("/buffer", offset);
		ASSERT_ZERO("/buffer", bytes);
	}

	/*
	 * Truncate this Buffer to the specified length.
	 */
	void truncate(size_t len)
	{
		if (length_ == len)
			return;
		ASSERT("/buffer", length_ > len);
		trim(length_ - len);
	}

	/*
	 * Split this Buffer into a vector of Buffers at each occurrance of the
	 * specified separator character sep.
	 */
	std::vector<Buffer> split(uint8_t ch, bool include_empty = false)
	{
		std::vector<Buffer> vec;

		if (empty()) {
			if (include_empty)
				vec.push_back(Buffer());
			return (vec);
		}

		for (;;) {
			size_t off;
			if (!find(ch, &off)) {
				/*
				 * Separator not found.
				 */
				if (!empty()) {
					vec.push_back(*this);
					clear();
				} else {
					if (include_empty)
						vec.push_back(Buffer());
				}
				return (vec);
			}

			/*
			 * Extract the data before the separator.
			 */
			if (off != 0) {
				Buffer buf;
				skip(off, &buf);
				vec.push_back(buf);
			} else {
				/*
				 * No data before the separator.
				 */
				if (include_empty)
					vec.push_back(Buffer());
			}

			/*
			 * Skip the separator.
			 */
			skip(1);
		}

		NOTREACHED("/buffer");
	}

	/*
	 * Merge a vector of Buffers into a single Buffer with the requested
	 * separator string sep.
	 *
	 * XXX Do we need/want include_empty?
	 */
	static Buffer join(const std::vector<Buffer>& vec, const std::string& sep = "")
	{
		if (vec.empty())
			return (Buffer());

		if (vec.size() == 1)
			return (vec.front());

		if (sep == "") {
			Buffer buf;
			std::vector<Buffer>::const_iterator it;
			for (it = vec.begin(); it != vec.end(); ++it)
				buf.append(*it);
			return (buf);
		}

		Buffer delim(sep);
		Buffer buf;
		std::vector<Buffer>::const_iterator it = vec.begin();
		for(;;) {
			buf.append(*it);
			if (++it == vec.end())
				break;
			buf.append(delim);
		}
		return (buf);
	}

	/*
	 * Output operator for strings.
	 */
	Buffer& operator<< (const std::string& str)
	{
		append(str);
		return (*this);
	}

	/*
	 * Output operator for Buffers.
	 */
	Buffer& operator<< (const Buffer& buf)
	{
		append(buf);
		return (*this);
	}

	/*
	 * Output operator for Buffer pointers.
	 */
	Buffer& operator<< (const Buffer *buf)
	{
		append(buf);
		return (*this);
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

namespace {
	template<typename T>
	Buffer& operator<< (Buffer& buf, T arg)
	{
		std::ostringstream os;
		os << arg;
		return (buf << os.str());
	}

	static inline Buffer& operator<< (Buffer& buf, const Buffer& src)
	{
		buf.append(src);
		return (buf);
	}

	static inline Buffer& operator<< (Buffer& buf, const Buffer *src)
	{
		buf.append(src);
		return (buf);
	}
}

#endif /* !COMMON_BUFFER_H */
