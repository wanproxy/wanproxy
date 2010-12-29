#ifndef	TIMER_H
#define	TIMER_H

class Timer {
	uintmax_t start_;
	uintmax_t stop_;

	std::vector<uintmax_t> samples_;
public:
	Timer(void)
	: start_(),
	  stop_(),
	  samples_()
	{ }

	~Timer()
	{ }

	void reset(void)
	{
		samples_.clear();
	}

	void start(void);
	void stop(void);

	uintmax_t sample(void) const
	{
		if (samples_.size() != 1)
			HALT("/timer") << "Requested 1 sample but " << samples_.size() << " available.";
		return (samples_[0]);
	}

	std::vector<uintmax_t> samples(void) const
	{
		return (samples_);
	}
};

#endif /* !TIMER_H */
