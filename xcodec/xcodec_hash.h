#ifndef	XCODEC_HASH_H
#define	XCODEC_HASH_H

class XCodecHash {
	struct RollingHash {
		uint32_t sum1_;					/* Really <16-bit.  */
		uint32_t sum2_;					/* Really <32-bit.  */
		uint32_t buffer_[XCODEC_SEGMENT_LENGTH];	/* Really >8-bit.  */

		RollingHash(void)
		: sum1_(0),
		  sum2_(0),
		  buffer_()
		{ }

		void add(uint32_t ch, unsigned start)
		{
			buffer_[start] = ch;

			sum1_ += ch;
			sum2_ += sum1_;
		}

		void reset(void)
		{
			sum1_ = 0;
			sum2_ = 0;
		}

		void roll(uint32_t ch, unsigned start)
		{
			uint32_t dead;

			dead = buffer_[start];

			sum1_ -= dead;
			sum2_ -= dead * XCODEC_SEGMENT_LENGTH;

			buffer_[start] = ch;

			sum1_ += ch;
			sum2_ += sum1_;
		}
	};

	RollingHash bytes_;
	RollingHash bits_;
	unsigned start_;
	unsigned length_;

public:
	XCodecHash(void)
	: bytes_(),
	  bits_(),
	  start_(0),
	  length_(0)
	{ }

	~XCodecHash()
	{ }

	void add(uint8_t ch)
	{
		unsigned bit = ffs(ch);
		unsigned word = (unsigned)ch + 1;

		ASSERT(length_ < XCODEC_SEGMENT_LENGTH);

		bytes_.add(word, start_);
		bits_.add(bit, start_);

		length_++;
		start_ = (start_ + 1) % XCODEC_SEGMENT_LENGTH;
	}

	void reset(void)
	{
		bytes_.reset();
		bits_.reset();

		length_ = 0;
		start_ = 0;
	}

	void roll(uint8_t ch)
	{
		unsigned bit = ffs(ch);
		unsigned word = (unsigned)ch + 1;

		ASSERT(length_ == XCODEC_SEGMENT_LENGTH);

		bytes_.roll(word, start_);
		bits_.roll(bit, start_);

		start_ = (start_ + 1) % XCODEC_SEGMENT_LENGTH;
	}

	/*
	 * XXX
	 * Need to write a compression function for this; get rid of the
	 * completely non-entropic bits, anyway, and try to mix the others.
	 *
	 * Need to look at what bits can even possibly be set in sum2_,
	 * looking at all possible ranges resulting from:
	 * 	128*data[0] + 127*data[1] + ... 1*data[127]
	 *
	 * It seems like since each data[] must have at least 1 bit set,
	 * there are a great many impossible values, and there is a large
	 * minimal value.  Should be easy to compress, and would rather
	 * have the extra bits changing from the normalized (i.e. non-zero)
	 * input and have to compress than have no bits changing...but
	 * perhaps those are extensionally-equivalent and that's just
	 * hocus pocus computer science.  Need to think more clearly and
	 * fully about it.
	 */
	uint64_t mix(void) const
	{
		ASSERT(length_ == XCODEC_SEGMENT_LENGTH);

		uint64_t bits_hash = (bits_.sum2_ << 10) | bits_.sum1_;
		uint64_t bytes_hash = (bytes_.sum2_ << 15) | bytes_.sum1_;
		return ((bits_hash << 36) | bytes_hash);
	}

	static uint64_t hash(const uint8_t *data)
	{
		XCodecHash xchash;
		unsigned i;

		for (i = 0; i < XCODEC_SEGMENT_LENGTH; i++)
			xchash.add(*data++);
		return (xchash.mix());
	}
};

#endif /* !XCODEC_HASH_H */
