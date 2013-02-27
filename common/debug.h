/*
 * Copyright (c) 2008-2012 Juli Mallett. All rights reserved.
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

#ifndef	COMMON_DEBUG_H
#define	COMMON_DEBUG_H

#if defined(NDEBUG)
#define	ASSERT(log, p)							\
	do {								\
		if (false)						\
			(void)(p);					\
	} while (0)
#else
#define	ASSERT(log, p)							\
	do {								\
		if (!(p)) {						\
			HALT((log)) << "Assertion (" << #p <<		\
					") failed at "			\
					<< __FILE__ << ':' << __LINE__	\
					<< " in function " 		\
					<< __PRETTY_FUNCTION__		\
					<< '.';				\
			for (;;)					\
				abort();				\
		}							\
	} while (0)
#endif

/*
 * XXX
 * Newer GCCs are horribly broken and seem to have become unable to detect
 * that HALT(x) << bar will prevent a function ever returning.  We add the
 * infinite loop here to help.
 */
#if defined(NDEBUG)
#define	NOTREACHED(log)							\
	abort()
#else
#define	NOTREACHED(log)							\
	do {								\
		ASSERT(log, "Should not be reached." == NULL);		\
		for (;;)						\
			abort();					\
	} while (0)
#endif

#endif /* !COMMON_DEBUG_H */
