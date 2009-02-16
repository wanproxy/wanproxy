.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.if defined(NETWORK_TEST)
.if !defined(SKIP_NETWORK_TESTS)
TEST=${NETWORK_TEST}
.endif
.endif

.if defined(TEST)
.if !defined(SKIP_TESTS)
PROG_CXX=${TEST}
SRCS+=${TEST}.cc

.if !target(regress)
regress: ${PROG_CXX}
	${.OBJDIR}/${PROG_CXX}
.endif
.endif
.endif

WARNS?=	3
NO_MAN?=duh

CFLAGS+=-I${TOPDIR}
.if defined(NDEBUG)
CFLAGS+=-DNDEBUG=1
.endif

.if defined(PROG_CXX)
__LIBRARIES!=echo ${USE_LIBS} | sort -u | xargs
.for _lib in ${__LIBRARIES}
.include "${TOPDIR}/${_lib}/lib.mk"
.endfor

.include <bsd.prog.mk>
.else
SUBDIR+=
.include <bsd.subdir.mk>
.endif

CFLAGS:=${CFLAGS:N-Wsystem-headers}
