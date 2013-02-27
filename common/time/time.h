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

#ifndef	COMMON_TIME_TIME_H
#define	COMMON_TIME_TIME_H

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
		ASSERT("/nano/time", *this >= b);

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

#endif /* !COMMON_TIME_TIME_H */
