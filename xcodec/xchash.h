#ifndef	XCHASH_H
#define	XCHASH_H

template<unsigned Tlength>
class XCHash {
	uint32_t sum1_;
	uint32_t sum2_;
	uint8_t xor_;
	uint8_t buffer_[Tlength];
	uint8_t start_;

public:
	XCHash(void)
	: sum1_(0),
	  sum2_(0),
	  xor_(0),
	  buffer_(),
	  start_(0)
	{
		memset(buffer_, 0, Tlength);
	}

	~XCHash()
	{ }

	void roll(uint8_t ch)
	{
		uint8_t dead;

		dead = buffer_[start_];

		sum1_ -= dead;
		sum2_ -= dead * Tlength;
		xor_ ^= dead;

		buffer_[start_] = ch;
		start_ = (start_ + 1) % Tlength;

		sum1_ += ch;
		sum2_ += sum1_;
		xor_ ^= ch;
	}

	static uint64_t mix(uint32_t a, uint32_t b, uint32_t c)
	{
		/*
		 * Bob Jenkins' mix function.
		 */
		a -= b;
		a -= c;
		a ^= c >> 13;

		b -= c;
		b -= a;
		b ^= a << 8;

		c -= a;
		c -= b;
		c ^= b >> 13;

		a -= b; 
		a -= c;
		a ^= c >> 12;

		b -= c;
		b -= a;
		b ^= a << 16;

		c -= a;
		c -= b;
		c ^= b >> 5;

		a -= b;
		a -= c;
		a ^= c >> 3;

		b -= c;
		b -= a;
		b ^= a << 10;

		c -= a;
		c -= b;
		c ^= b >> 15;

		return (c);
	}

	uint64_t mix(void) const
	{
		uint64_t sum;

		sum = 0;
		sum |= mix(sum1_, sum2_, (sum2_ - sum1_) + buffer_[start_]);
		sum |= mix(Tlength, buffer_[start_] << 8 | xor_, sum) << 32;

		return (sum);
	}

	static uint64_t hash(const uint8_t *data)
	{
		XCHash<Tlength> hash;
		unsigned i;

		for (i = 0; i < Tlength; i++)
			hash.roll(data[i]);
		return (hash.mix());
	}
};

#endif /* !XCHASH_H */
