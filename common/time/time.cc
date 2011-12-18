#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <common/time/time.h>

NanoTime
NanoTime::current_time(void)
{
	NanoTime nt;
	int rv;

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
	struct timespec ts;

	rv = ::clock_gettime(CLOCK_MONOTONIC, &ts);
	ASSERT("/nano/time/posix", rv != -1);

	nt.seconds_ = ts.tv_sec;
	nt.nanoseconds_ = ts.tv_nsec;
#else
	struct timeval tv;

	rv = ::gettimeofday(&tv, NULL);
	ASSERT("/nano/time/gtod", rv != -1);
	nt.seconds_ = tv.tv_sec;
	nt.nanoseconds_ = tv.tv_usec * 1000;
#endif

	return (nt);
}
