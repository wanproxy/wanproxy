#ifndef	XCODEC_HASH_H
#define	XCODEC_HASH_H

class XCodecHash {
	struct RollingHash {
		uint32_t sum1_;					/* Really <16-bit.  */
		uint32_t sum2_;					/* Really <32-bit.  */
		uint32_t buffer_[XCODEC_SEGMENT_LENGTH];	/* Really 8-bit.  */

		RollingHash(void)
		: sum1_(0),
		  sum2_(0),
		  buffer_()
		{
			memset(buffer_, 0, XCODEC_SEGMENT_LENGTH);
		}

		void roll(uint8_t ch, unsigned start)
		{
			uint8_t dead;

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
	uint8_t start_;

public:
	XCodecHash(void)
	: bytes_(),
	  bits_(),
	  start_(0)
	{
	}

	~XCodecHash()
	{ }

	void roll(uint8_t ch)
	{
		bytes_.roll(ch + 1, start_);
		bits_.roll(ffs(ch), start_);

		start_ = (start_ + 1) % XCODEC_SEGMENT_LENGTH;
	}

	uint64_t mix(void) const
	{
		uint64_t bits_hash = (bits_.sum2_ << 10) | bits_.sum1_;
		uint64_t bytes_hash = (bytes_.sum2_ << 15) | bytes_.sum1_;
		return ((bits_hash << 36) | bytes_hash);
	}

	static uint64_t hash(const uint8_t *data)
	{
		XCodecHash xchash;
		unsigned i;

		for (i = 0; i < XCODEC_SEGMENT_LENGTH; i++)
			xchash.roll(*data++);
		return (xchash.mix());
	}
};

#endif /* !XCODEC_HASH_H */
