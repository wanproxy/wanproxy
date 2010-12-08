VPATH+=	${TOPDIR}/event

SRCS+=	event_poll.cc
SRCS+=	event_system.cc
SRCS+=	timeout.cc

ifndef USE_POLL
ifeq "${OSNAME}" "Darwin"
USE_POLL=	kqueue
endif

ifeq "${OSNAME}" "FreeBSD"
USE_POLL=	kqueue
endif

ifeq "${OSNAME}" "OpenBSD"
USE_POLL=	kqueue
endif

ifeq "${OSNAME}" "Linux"
USE_POLL=	epoll
endif

ifeq "${OSNAME}" "Interix"
USE_POLL=	select
endif

ifeq "${OSNAME}" "SunOS"
USE_POLL=	port
endif

ifndef USE_POLL
USE_POLL=	poll
endif
endif

ifeq "${OSNAME}" "Linux"
# Required for clock_gettime(3).
LDADD+=		-lrt
endif

SRCS+=	event_poll_${USE_POLL}.cc

__USE_POLL:=$(shell echo ${USE_POLL} | tr a-z A-Z)
