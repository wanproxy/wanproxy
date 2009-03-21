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
	"${.OBJDIR}/${PROG_CXX}"
.endif
.endif
.endif

.if !target(regress)
regress: ${PROG_CXX}
.endif

.PHONY: regress

CFLAGS+=-I"${TOPDIR}"
.if defined(NDEBUG)
CFLAGS+=-DNDEBUG=1
.else
CFLAGS+=-g
.endif

CFLAGS+=-W -Wall -Werror
CFLAGS+=-Wno-system-headers
#CFLAGS+=-Wno-unused-parameter
CFLAGS+=-Wpointer-arith -Wreturn-type -Wcast-qual -Wwrite-strings -Wswitch -Wshadow -Wcast-align -Wunused-parameter -Wchar-subscripts
#CFLAGS+=-Winline

.if defined(PROG_CXX)
__LIBRARIES!=echo ${USE_LIBS} | sort -u | xargs
.for _lib in ${__LIBRARIES}
.include "${TOPDIR}/${_lib}/lib.mk"
.endfor

OBJS+=  ${SRCS:R:S/$/.o/g}

all: ${PROG_CXX}

${PROG_CXX}: ${OBJS}
	${CXX} ${CXXFLAGS} ${LDFLAGS} -o ${.TARGET} ${OBJS} ${LDADD}

clean:
	rm -f ${PROG_CXX} ${OBJS}

.include <bsd.obj.mk>
.else
SUBDIR+=
.include <bsd.subdir.mk>
.endif
