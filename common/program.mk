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
PROGRAM=${TEST}
SRCS+=${TEST}.cc

.if !target(regress)
regress: ${PROGRAM}
	${.OBJDIR}/${PROGRAM}
.endif
.endif
.endif

.if !target(regress)
regress: ${PROGRAM}
.endif

.PHONY: regress

OSNAME!=uname -s

CFLAGS+=-I${TOPDIR}
.if defined(NDEBUG)
CFLAGS+=-DNDEBUG=1
.else
.if ${OSNAME} != "SunOS"
CFLAGS+=-g
.endif
.endif

#CFLAGS+=--std gnu++0x
#CFLAGS+=-pedantic
CFLAGS+=-Wno-deprecated
CFLAGS+=-W -Wall
.if ${OSNAME} != "OpenBSD"
CFLAGS+=-Werror
.endif
CFLAGS+=-Wno-system-headers
#CFLAGS+=-Wno-unused-parameter
CFLAGS+=-Wpointer-arith -Wreturn-type -Wcast-qual -Wwrite-strings -Wswitch -Wshadow -Wcast-align -Wunused-parameter -Wchar-subscripts
#CFLAGS+=-Winline

.if defined(PROGRAM)
__LIBRARIES!=echo ${USE_LIBS} | sort -u | xargs
.for _lib in ${__LIBRARIES}
.include "${TOPDIR}/${_lib}/lib.mk"
.if defined(${_lib:U}_REQUIRES)
.for _lib2 in ${${_lib:U}_REQUIRES}
.if empty(__LIBRARIES:M${_lib2})
.error "Inclusion of library ${_lib} requires library ${_lib2}."
.endif
.endfor
.endif
.endfor

OBJS+=  ${SRCS:R:S/$/.o/g}

.MAIN: ${PROGRAM}
all: ${PROGRAM}

${PROGRAM}: ${OBJS}
	${CXX} ${CXXFLAGS} ${CFLAGS} ${LDFLAGS} -o ${.TARGET} ${OBJS} ${LDADD}

.cc.o:
	${CXX} ${CXXFLAGS} ${CFLAGS} -c -o ${.TARGET} ${.IMPSRC}

clean:
	rm -f ${PROGRAM} ${OBJS}
.else
SUBDIR+=
.include <bsd.subdir.mk>
.endif
