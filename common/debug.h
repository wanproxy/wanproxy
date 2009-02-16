#ifndef	DEBUG_H
#define	DEBUG_H

#if defined(NDEBUG)
#define	ASSERT(p)							\
	do {								\
		if (false)						\
			(void)(p);					\
	} while (0)
#else
#define	ASSERT(p)							\
	do {								\
		if (!(p))						\
			HALT("/assert") << "Assertion ("		\
					<< #p << ") failed at "		\
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
#define	NOTREACHED()							\
	abort()
#else
#define	NOTREACHED()							\
	do {								\
		ASSERT("Should not be reached." == NULL);		\
		for (;;)						\
			continue;					\
	} while (0)
#endif

#endif /* !DEBUG_H */
