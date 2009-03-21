#ifndef	XCODEC_HASH_H
#define	XCODEC_HASH_H

#include <alg/hash/adler64.h>

template<unsigned Tlength>
class XCodecHash {
	Adler64<Tlength> adler64_;
	uint8_t xor_;
	uint8_t buffer_[Tlength];
	uint8_t start_;

public:
	XCodecHash(void)
	: adler64_(),
	  xor_(0),
	  buffer_(),
	  start_(0)
	{
		memset(buffer_, 0, Tlength);
	}

	~XCodecHash()
	{ }

	void roll(uint8_t ch)
	{
		adler64_ += ch;

		uint8_t dead;

		dead = buffer_[start_];

		xor_ ^= dead;

		buffer_[start_] = ch;
		start_ = (start_ + 1) % Tlength;

		xor_ ^= ch;
	}

	static uint32_t mix(uint32_t a, uint32_t b, uint32_t c)
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

	struct mix_functor {
		uint8_t c_;
		uint8_t x_;

		mix_functor(const uint8_t& c, const uint8_t& x)
		: c_(c),
		  x_(x)
		{ }

		uintmax_t operator() (const uint32_t& a, const uint32_t& b)
		{
			uint64_t sum;

			sum = 0;
			sum |= mix(a, b, (b - a) + c_);
			sum |= (uint64_t)mix(Tlength, c_ << 8 | x_, (uint32_t)sum) << 32;

			return (sum);
		}
	};

	uint64_t mix(void) const
	{
		mix_functor f(buffer_[start_], xor_);

		return (adler64_.mix(f));
	}

	static uint64_t hash(const uint8_t *data)
	{
		XCodecHash<Tlength> hash;
		unsigned i;

		for (i = 0; i < Tlength; i++)
			hash.roll(data[i]);
		return (hash.mix());
	}
};

#endif /* !XCODEC_HASH_H */
