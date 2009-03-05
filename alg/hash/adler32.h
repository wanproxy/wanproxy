#ifndef	ADLER32_H
#define	ADLER32_H

/*
 * A modulus-free rolling Adler32.
 */

template<unsigned Tlength>
class Adler32 {
	uint16_t sum1_;
	uint16_t sum2_;
	uint8_t buffer_[Tlength];
	uint8_t start_;

public:
	Adler32(void)
	: sum1_(0),
	  sum2_(0),
	  buffer_(),
	  start_(0)
	{
		memset(buffer_, 0, Tlength);
	}

	~Adler32()
	{ }

	Adler32& operator += (const uint8_t& ch)
	{
		uint8_t dead;

		dead = buffer_[start_];

		sum1_ -= dead;
		sum2_ -= dead * Tlength;

		buffer_[start_] = ch;
		start_ = (start_ + 1) % Tlength;

		sum1_ += ch;
		sum2_ += sum1_;

		return (*this);
	}

	uint32_t sum(void) const
	{
		return ((b << 16) | a);
	}

	template<typename T>
	uintmax_t mix(T mix_function) const
	{
		return (mix_function(sum1_, sum2_));
	}
};

#endif /* !ADLER32_H */
