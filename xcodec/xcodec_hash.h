#ifndef	XCODEC_HASH_H
#define	XCODEC_HASH_H

#include <alg/hash/adler64.h>

template<unsigned Tlength>
class XCodecHash {
	Adler64<Tlength> adler64_;

public:
	XCodecHash(void)
	: adler64_()
	{ }

	~XCodecHash()
	{ }

	void roll(uint8_t ch)
	{
		adler64_ += ch;
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
			uint64_t sum;

			sum = 0;
			sum |= mix(a, b, b - a);
			sum |= (uint64_t)mix(Tlength, b ^ a, (uint32_t)sum) << 32;

			return (sum);
		}
	};

	uint64_t mix(void) const
	{
		mix_functor f;

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
