#include <sys/time.h>

#include <vector>

#include <common/timer.h>

void
Timer::start(void)
{
	struct timeval tv;
	int rv;

	rv = gettimeofday(&tv, NULL);
	if (rv == -1)
		HALT("/timer") << "Could not gettimeofday.";
	start_ = (tv.tv_sec * 1000 * 1000) + tv.tv_usec;
}

void
Timer::stop(void)
{
	struct timeval tv;
	int rv;

	rv = gettimeofday(&tv, NULL);
	if (rv == -1)
		HALT("/timer") << "Could not gettimeofday.";
	stop_ = (tv.tv_sec * 1000 * 1000) + tv.tv_usec;

	samples_.push_back(stop_ - start_);
}

uintmax_t
Timer::sample(void) const
{
	if (samples_.size() != 1)
		HALT("/timer") << "Requested 1 sample but " << samples_.size() << " available.";
	return (samples_[0]);
}

std::vector<uintmax_t>
Timer::samples(void) const
{
	return (samples_);
}
