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
	${PWD}/${PROGRAM}
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

CFLAGS+=-I${TOPDIR}
ifdef NDEBUG
CFLAGS+=-DNDEBUG=1
else
ifneq "${OSNAME}" "SunOS"
CFLAGS+=-g
endif
endif

#CFLAGS+=--std gnu++0x
#CFLAGS+=-pedantic
CFLAGS+=-Wno-deprecated
CFLAGS+=-W -Wall
ifneq "${OSNAME}" "OpenBSD"
CFLAGS+=-Werror
endif
CFLAGS+=-Wno-system-headers
#CFLAGS+=-Wno-unused-parameter
CFLAGS+=-Wpointer-arith -Wreturn-type -Wcast-qual -Wwrite-strings -Wswitch -Wshadow -Wcast-align -Wunused-parameter -Wchar-subscripts
#CFLAGS+=-Winline

#
# XXX
# We completely ignore dependencies for now since this seems dodgy.
#
# define __library_check_dependency
# _lib:=$(1)
# _lib2:=$(2)
# ifeq ${lib} ${lib2}
# _library_found:=1
# endif
# endef
# 
# define __library_dependency
# _lib:=$(1)
# _lib2:=$(2)
# _library_found:=0
# $(foreach _lib3, ${USE_LIBS}, $(eval $(call __library_check_dependency, ${_lib2}, ${_lib3})))
# ifneq ${_library_found} 1
# $(error "Inclusion of library ${_lib} requires library ${_lib2}")
# endif
# endef

define __library_include
_lib:=$(1)
# _LIB:=$(shell echo ${_lib} | tr a-z A-Z)
 
include ${TOPDIR}/${_lib}/lib.mk
 
# ifdef ${_LIB}_REQUIRES
# $(foreach _lib2, ${${_LIB}_REQUIRES}, $(eval $(call __library_dependency, ${_lib}, ${_lib2})))
# endif
endef

$(foreach _lib, ${USE_LIBS}, $(eval $(call __library_include, ${_lib})))

OBJS+=  ${SRCS:.cc=.o}

${PROGRAM}: ${OBJS}
	${CXX} ${CXXFLAGS} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJS} ${LDADD}

.cc.o:
	${CXX} ${CXXFLAGS} ${CFLAGS} -c -o $@ $<

clean:
	rm -f ${PROGRAM} ${OBJS}
