/*
 * Copyright (c) 2011 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
