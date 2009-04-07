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

	void start(void);
	void stop(void);

	uintmax_t sample(void) const;
	std::vector<uintmax_t> samples(void) const;
};

#endif /* !TIMER_H */
