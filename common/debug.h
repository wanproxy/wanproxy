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
		if (!(p))						\
			HALT((log)) << "Assertion (" << #p <<		\
					") failed at "			\
					<< __FILE__ << ':' << __LINE__	\
					<< " in function " 		\
					<< __PRETTY_FUNCTION__		\
					<< '.';				\
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
