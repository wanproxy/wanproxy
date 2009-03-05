#ifndef	TIMEOUT_H
#define	TIMEOUT_H

#include <map>

class TimeoutQueue {
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

	typedef std::map<NanoTime, CallbackQueue> timeout_map_t;

	LogHandle log_;
	timeout_map_t timeout_queue_;
public:
	TimeoutQueue(void)
	: log_("/event/timeout/queue"),
	  timeout_queue_()
	{ }

	~TimeoutQueue()
	{ }

	bool empty(void) const
	{
		/*
		 * Since we allow elements within each CallbackQueue to be
		 * cancelled, this may be incorrect, but will be corrected by
		 * the perform method.  The same caveat applies to ready() and
		 * interval().  Luckily, it should be seldom that a user is
		 * cancelling a callback that has not been invoked by perform().
		 */
		return (timeout_queue_.empty());
	}

	Action *append(uintmax_t, Callback *);
	uintmax_t interval(void) const;
	void perform(void);
	bool ready(void) const;
};

#endif /* !TIMEOUT_H */
