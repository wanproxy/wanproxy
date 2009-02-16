#ifndef	BUFFER_H
#define	BUFFER_H

#include <deque>

struct iovec;

#define	BUFFER_SEGMENT_SIZE	(128)

typedef	uint8_t	buffer_segment_size_t;

class BufferSegment {
	uint8_t data_[BUFFER_SEGMENT_SIZE];
	buffer_segment_size_t offset_;
	buffer_segment_size_t length_;
	unsigned ref_;
public:
	BufferSegment(void)
	: data_(),
	  offset_(0),
	  length_(0),
	  ref_(1)
	{
	}

	~BufferSegment()
	{
		ASSERT(ref_ == 0);
	}

	void ref(void)
	{
		ASSERT(ref_ != 0);
		ref_++;
	}

	void unref(void)
	{
		if (--ref_ == 0)
			delete this;
	}

	uint8_t *head(void)
	{
		ASSERT(ref_ == 1);
		return (&data_[offset_]);
	}

	uint8_t *tail(void)
	{
		ASSERT(ref_ == 1);
		return (&data_[offset_ + length_]);
	}

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

	BufferSegment *append(uint8_t ch)
	{
		return (append(&ch, 1));
	}

	BufferSegment *append(const std::string& str)
	{
		return (append((const uint8_t *)str.c_str(), str.length()));
	}

	size_t avail(void) const
	{
		return (BUFFER_SEGMENT_SIZE - length());
	}

	BufferSegment *copy(void) const
	{
		BufferSegment *seg = new BufferSegment();
		memcpy(seg->data_, data_, BUFFER_SEGMENT_SIZE);
		seg->offset_ = offset_;
		seg->length_ = length_;
		return (seg);
	}

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

	const uint8_t *data(void) const
	{
		return (&data_[offset_]);
	}

	const uint8_t *end(void) const
	{
		return (&data_[offset_ + length_]);
	}

	size_t length(void) const
	{
		return (length_);
	}

	void pullup(void)
	{
		if (offset_ == 0)
			return;
		memmove(data_, data(), length());
		offset_ = 0;
	}

	void set_length(size_t len)
	{
		ASSERT(ref_ == 1);
		ASSERT(offset_ == 0);
		ASSERT(len <= BUFFER_SEGMENT_SIZE);
		length_ = len;
	}

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

	bool match(const uint8_t *buf, size_t len) const
	{
		if (len != length())
			return (false);
		return (prefix(buf, len));
	}

	bool match(const BufferSegment *seg) const
	{
		return (match(seg->data(), seg->length()));
	}

	bool match(const std::string& str) const
	{
		return (match((const uint8_t *)str.c_str(), str.length()));
	}

	bool prefix(const uint8_t *buf, size_t len) const
	{
		return (memcmp(data(), buf, len) == 0);
	}

	bool prefix(const std::string& str) const
	{
		return (prefix((const uint8_t *)str.c_str(), str.length()));
	}
};

class Buffer {
public:
	typedef	std::deque<BufferSegment *> segment_list_t;

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
	Buffer(void)
	: length_(0),
	  data_()
	{
	}

	Buffer(const uint8_t *buf, size_t len)
	: length_(0),
	  data_()
	{
		append(buf, len);
	}

	Buffer(const Buffer& source)
	: length_(0),
	  data_()
	{
		append(source);
	}

	Buffer(const Buffer& source, size_t len)
	: length_(0),
	  data_()
	{
		append(source, len);
	}

	Buffer(const std::string& str)
	: length_(0),
	  data_()
	{
		append((const uint8_t *)str.c_str(), str.length());
	}

	~Buffer()
	{
		clear();
	}

	Buffer& operator= (const Buffer& source)
	{
		clear();
		append(&source);
		return (*this);
	}

	Buffer& operator= (const std::string& str)
	{
		clear();
		append(str);
		return (*this);
	}

	void append(const std::string& str)
	{
		append((const uint8_t *)str.c_str(), str.length());
	}

	void append(const Buffer& buf)
	{
		segment_list_t::const_iterator it;

		for (it = buf.data_.begin(); it != buf.data_.end(); ++it)
			append(*it);
	}

	void append(const Buffer& buf, size_t len)
	{
		if (len == 0)
			return;

		ASSERT(len <= buf.length());
		append(buf);
		if (buf.length() != len)
			trim(buf.length() - len);
	}

	void append(const Buffer *buf)
	{
		append(*buf);
	}

	void append(const Buffer *buf, size_t len)
	{
		append(*buf, len);
	}

	void append(BufferSegment *seg)
	{
		ASSERT(seg->length() != 0);
		seg->ref();
		data_.push_back(seg);
		length_ += seg->length();
	}

	void append(uint8_t ch)
	{
		append(&ch, 1);
	}

	void append(const uint8_t *buf, size_t len)
	{
		BufferSegment *seg;
		unsigned o;

		/*
		 * If we are adding less than a single segment worth of data,
		 * try to append it to the end of the last segment.
		 *
		 * XXX Is this special case worthwhile?  It needs to check for
		 * sharing anyway, and COWing is probably slower than just
		 * creating a new segment?
		 */
		if (len < BUFFER_SEGMENT_SIZE && !data_.empty()) {
			segment_list_t::iterator it;

			it = --data_.end();
			seg = *it;
			if (seg->avail() >= len) {
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

	size_t copyout(uint8_t *dst, size_t dstsize) const
	{
		return (copyout(dst, 0, dstsize));
	}

	size_t copyout(BufferSegment *seg, size_t len) const
	{
		ASSERT(seg->avail() >= len);
		len = copyout(seg->tail(), len);
		seg->set_length(seg->length() + len);
		return (len);
	}

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

	SegmentIterator segments(void) const
	{
		return (SegmentIterator(data_));
	}

	bool empty(void) const
	{
		return (data_.empty());
	}

	bool equal(Buffer *buf) const
	{
		if (length() != buf->length())
			return (false);
		return (prefix(buf));
	}

	bool equal(const uint8_t *buf, size_t len) const
	{
		if (len != length())
			return (false);
		return (prefix(buf, len));
	}

	bool equal(const std::string& str) const
	{
		Buffer tmp(str);
		return (equal(&tmp));
	}

	template<typename T>
	void escape(uint8_t esc, T predicate)
	{
		segment_list_t escaped_data;
		segment_list_t::iterator it;
		bool any_escapes;

		any_escapes = false;

		for (it = data_.begin(); it != data_.end(); ++it) {
			BufferSegment *seg = *it;
			const uint8_t *p;
			bool escaped;

			escaped = false;

			for (p = seg->data(); p < seg->end(); p++) {
				if (!predicate(*p))
					continue;
				BufferSegment *nseg;

				nseg = new BufferSegment();
				if (p != seg->data())
					nseg->append(seg->data(),
						     (p - seg->data()));
				while (p < seg->end()) {
					uint8_t ch = *p;
					if (predicate(ch)) {
						if (nseg->avail() == 0) {
							escaped_data.push_back(nseg);
							nseg = new BufferSegment();
						}
						nseg->append(esc);
						length_++;
					}
					if (nseg->avail() == 0) {
						escaped_data.push_back(nseg);
						nseg = new BufferSegment();
					}
					nseg->append(ch);
					p++;
				}
				escaped_data.push_back(nseg);
				escaped = true;
				break;
			}
			if (!escaped)
				escaped_data.push_back(seg);
			else {
				seg->unref();
				any_escapes = true;
			}
		}
		if (any_escapes) {
			data_.clear();
			data_ = escaped_data;
		}
	}

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

			for (p = seg->data(); p < seg->end(); p++) {
				if (*p != ch) {
					if (offset++ == limit)
						return (false);
					continue;
				}
				*offsetp = offset;
				return (true);
			}
		}
		return (false);
	}

	size_t length(void) const
	{
		return (length_);
	}

	void moveout(uint8_t *dst, unsigned offset, size_t dstsize)
	{
		ASSERT(length() >= offset + dstsize);
		size_t copied = copyout(dst, offset, dstsize);
		ASSERT(copied == dstsize);
		skip(offset + dstsize);
	}

	void moveout(uint8_t *dst, size_t dstsize)
	{
		moveout(dst, 0, dstsize);
	}

	void moveout(Buffer *dst, unsigned offset, size_t dstsize)
	{
		ASSERT(length() >= offset + dstsize);
		if (offset != 0)
			skip(offset);
		dst->append(this, dstsize);
		skip(dstsize);
	}

	void moveout(Buffer *dst, size_t dstsize)
	{
		moveout(dst, 0, dstsize);
	}

	uint8_t peek(void) const
	{
		ASSERT(length_ != 0);

		const BufferSegment *seg = data_.front();
		return (seg->data()[0]);
	}

	bool prefix(const std::string& str) const
	{
		ASSERT(str.length() > 0);
		if (str.length() > length())
			return (false);
		Buffer tmp(str);
		return (prefix(&tmp));
	}

	bool prefix(const uint8_t *buf, size_t len) const
	{
		ASSERT(len > 0);
		if (len > length())
			return (false);
		Buffer tmp(buf, len);
		return (prefix(&tmp));
	}

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
			pfxseg = *pfx;
		}
		return (true);
	}

	/*
	 * XXX Keep in sync with trim().
	 */
	void skip(size_t bytes)
	{
		segment_list_t::iterator it;
		unsigned skipped;

		if (bytes == length())
			clear();

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
	 * XXX Keep in sync with skip().
	 */
	void trim(size_t bytes)
	{
		segment_list_t::iterator it;
		unsigned trimmed;

		if (bytes == length())
			clear();

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

	/* And now the fancypants stuff.  */
	template<typename T>
	void foreach_byte(T& f) const
	{
		segment_list_t::const_iterator it;

		for (it = data_.begin(); it != data_.end(); ++it) {
			const BufferSegment *seg = *it;
			const uint8_t *p;

			for (p = seg->data(); p < seg->end(); p++)
				f(*p);
		}
	}

	template<typename T>
	void foreach_segment(T& f) const
	{
		segment_list_t::const_iterator it;

		for (it = data_.begin(); it != data_.end(); ++it)
			f(*it);
	}

	size_t fill_iovec(struct iovec *, size_t) const;
};

std::ostream& operator<< (std::ostream&, const Buffer *);
std::ostream& operator<< (std::ostream&, const Buffer&);

#endif /* !BUFFER_H */
