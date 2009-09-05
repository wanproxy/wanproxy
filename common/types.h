#ifndef	COMMON_TYPES_H
#define	COMMON_TYPES_H

#if !defined(__OPENNT)
#include <stdint.h> /* XXX cstdint? */
#endif

#if defined(__OPENNT)
typedef	unsigned long long	uintmax_t;
typedef	long long		intmax_t;
typedef	unsigned int		uintptr_t; /* XXX Appears to be in stddef.h!  */
#endif

#endif /* !COMMON_TYPES_H */
