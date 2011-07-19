#ifndef	COMMON_TIME_H
#define	COMMON_TIME_H /* 4/4 */

#include <map>

struct NanoTime {
	uintmax_t seconds_;
	uintmax_t nanoseconds_;

	NanoTime(void)
	: seconds_(0),
	  nanoseconds_(0)
	{ }

	NanoTime(const NanoTime& src)
	: seconds_(src.seconds_),
	  nanoseconds_(src.nanoseconds_)
	{ }

	bool operator< (const NanoTime& b) const
	{
		if (seconds_ == b.seconds_)
			return (nanoseconds_ < b.nanoseconds_);
		return (seconds_ < b.seconds_);
	}

	bool operator> (const NanoTime& b) const
	{
		if (seconds_ == b.seconds_)
			return (nanoseconds_ > b.nanoseconds_);
		return (seconds_ > b.seconds_);
	}

	bool operator<= (const NanoTime& b) const
	{
		if (seconds_ == b.seconds_)
			return (nanoseconds_ <= b.nanoseconds_);
		return (seconds_ <= b.seconds_);
	}

	bool operator>= (const NanoTime& b) const
	{
		if (seconds_ == b.seconds_)
			return (nanoseconds_ >= b.nanoseconds_);
		return (seconds_ >= b.seconds_);
	}

	NanoTime& operator+= (const NanoTime& b)
	{
		seconds_ += b.seconds_;
		nanoseconds_ += b.nanoseconds_;

		if (nanoseconds_ >= 1000000000) {
			seconds_++;
			nanoseconds_ -= 1000000000;
		}

		return (*this);
	}

	NanoTime& operator-= (const NanoTime& b)
	{
		ASSERT(*this >= b);

		if (nanoseconds_ < b.nanoseconds_) {
			nanoseconds_ += 1000000000;
			seconds_ -= 1;
		}

		seconds_ -= b.seconds_;
		nanoseconds_ -= b.nanoseconds_;

		return (*this);
	}

	static NanoTime current_time(void);
};

#endif /* !COMMON_TIME_H */
