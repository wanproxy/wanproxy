/*
 * Copyright (c) 2010-2011 Juli Mallett. All rights reserved.
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

#ifndef	COMMON_THREAD_ATOMIC_H
#define	COMMON_THREAD_ATOMIC_H

template<typename T>
class Atomic {
	T val_;
public:
	Atomic(void)
	: val_()
	{ }

	template<typename Ta>
	Atomic(Ta arg)
	: val_(arg)
	{ }

	~Atomic()
	{ }

	/*
	 * Note that these are deliberately not operator overloads, to force
	 * deliberate use of this class.
	 */

#if defined(__GNUC__)
	template<typename Ta>
	T add(Ta arg)
	{
		return (__sync_fetch_and_add(&val_, arg));
	}

	template<typename Ta>
	T subtract(Ta arg)
	{
		return (__sync_fetch_and_sub(&val_, arg));
	}

	template<typename Ta>
	T set(Ta arg)
	{
		return (__sync_fetch_and_or(&val_, arg));
	}

	template<typename Ta>
	T mask(Ta arg)
	{
		return (__sync_fetch_and_and(&val_, arg));
	}

	template<typename Ta>
	T clear(Ta arg)
	{
		return (__sync_fetch_and_and(&val_, ~arg));
	}

	template<typename To, typename Tn>
	T cmpset(Ta oldval, Tn newval)
	{
		return (__sync_val_compare_and_swap(&val_, oldval, newval));
	}
#else
#error "No support for atomic operations for your compiler.  Why not add some?"
#endif
};

#endif /* !COMMON_THREAD_ATOMIC_H */
