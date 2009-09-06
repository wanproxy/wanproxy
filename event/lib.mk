.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.PATH: ${TOPDIR}/event

SRCS+=	event_poll.cc
SRCS+=	event_system.cc
SRCS+=	timeout.cc

.if !defined(USE_POLL)
.if ${OSNAME} == "Darwin" || ${OSNAME} == "FreeBSD"
USE_POLL=	kqueue
.elif ${OSNAME} == "Linux"
USE_POLL=	epoll
.elif ${OSNAME} == "Interix"
USE_POLL=	select
.else
USE_POLL=	poll
.endif
.endif

.if ${OSNAME} == "Linux"
# Required for clock_gettime(3).
LDADD+=		-lrt
.endif

SRCS+=	event_poll_${USE_POLL}.cc

.if ${USE_POLL} == "kqueue"
CFLAGS+=-DUSE_POLL_KQUEUE
.elif ${USE_POLL} == "poll"
CFLAGS+=-DUSE_POLL_POLL
.elif ${USE_POLL} == "select"
CFLAGS+=-DUSE_POLL_SELECT
.elif ${USE_POLL} == "epoll"
CFLAGS+=-DUSE_POLL_EPOLL
.else
.error "Unsupported poll mechanism."
.endif
