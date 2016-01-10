VPATH+=	${TOPDIR}/event

SRCS+=	callback.cc
SRCS+=	callback_thread.cc
SRCS+=	destroy_thread.cc
SRCS+=	event_main.cc
SRCS+=	event_poll.cc
SRCS+=	event_system.cc
SRCS+=	timeout_queue.cc
SRCS+=	timeout_thread.cc

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
