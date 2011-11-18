ifndef TOPDIR
$(error "TOPDIR must be defined")
endif

ifdef NETWORK_TEST
TEST=${NETWORK_TEST}
endif

ifdef SKIP_NETWORK_TESTS
SKIP_TESTS=${SKIP_NETWORK_TESTS}
endif

ifdef TEST
ifndef SKIP_TESTS
PROGRAM=${TEST}
SRCS+=${TEST}.cc

all: ${PROGRAM}

# Build and run regression tests.
regress: ${PROGRAM}
ifdef TEST_WRAPPER
	${TEST_WRAPPER} ${PWD}/${PROGRAM}
else
	${PWD}/${PROGRAM}
endif
else
# Build but don't run regression tests.
regress: ${PROGRAM}
endif
else
ifndef PROGRAM
$(error "Must have a program to build.")
endif

all: ${PROGRAM}

# Not a regression test, do nothing.
regress:
	@true
endif

.PHONY: regress

OSNAME:=$(shell uname -s)

CFLAGS+=-pipe
CPPFLAGS+=-I${TOPDIR}
ifdef NDEBUG
CFLAGS+=-O2
CPPFLAGS+=-DNDEBUG=1
else
CFLAGS+=-O
ifneq "${OSNAME}" "SunOS"
CFLAGS+=-g
endif
endif

#CFLAGS+=--std gnu++0x
#CFLAGS+=-pedantic
CXXFLAGS+=-Wno-deprecated
CFLAGS+=-W -Wall
ifneq "${OSNAME}" "OpenBSD"
CFLAGS+=-Werror
endif
CFLAGS+=-Wno-system-headers
#CFLAGS+=-Wno-unused-parameter
CFLAGS+=-Wno-uninitialized
CFLAGS+=-Wpointer-arith -Wreturn-type -Wcast-qual -Wwrite-strings -Wswitch -Wshadow -Wcast-align -Wunused-parameter -Wchar-subscripts -Wreorder
#CFLAGS+=-Winline

$(foreach _lib, ${USE_LIBS}, $(eval include ${TOPDIR}/$(strip ${_lib})/lib.mk))

define __library_conditionals
ifdef CFLAGS_${1}
CFLAGS+=${CFLAGS_${1}}
endif
ifdef SRCS_${1}
SRCS+=	${SRCS_${1}}
endif
endef

$(foreach _lib, ${USE_LIBS}, $(eval $(call __library_conditionals,$(subst /,_,${_lib}))))

OBJS+=  $(patsubst %.cc,%.o,$(patsubst %.c,%.o,${SRCS}))

${PROGRAM}: ${OBJS}
	${CXX} ${CXXFLAGS} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJS} ${LDADD}

.cc.o:
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${CFLAGS} -c -o $@ $<

.c.o:
	${CC} ${CPPFLAGS} ${CFLAGS} -c -o $@ $<

clean:
	rm -f ${PROGRAM} ${OBJS}
