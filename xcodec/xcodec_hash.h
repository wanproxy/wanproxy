#ifndef	XCODEC_HASH_H
#define	XCODEC_HASH_H

#include <alg/hash/adler32.h>

template<unsigned Tlength>
class XCodecHash {
	Adler32<Tlength> a_, b_;

public:
	XCodecHash(void)
	: a_(),
	  b_()
	{ }

	~XCodecHash()
	{ }

	void roll(uint8_t ch)
	{
		unsigned bit;

		a_ += ch;
		bit = ffs(ch);
		ch >>= bit;
		if (bit == 0) {
			b_ += 8 * ch * ch;
		} else {
			b_ += bit * ch * (ch << bit);
		}
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
		uintmax_t operator() (const uint32_t& a, const uint32_t& b)
		{
			return (mix(a, b, b - a));
		}
	};

	uint64_t mix(void) const
	{
		mix_functor f;

		uint32_t m = f(a_.mix(f), b_.mix(f));
		uint32_t n = f(m, b_.mix(f));

		return (((uint64_t)m << 32) | n);
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
