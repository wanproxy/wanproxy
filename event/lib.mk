.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.PATH: ${TOPDIR}/event

SRCS+=	event_poll.cc
SRCS+=	event_system.cc
SRCS+=	timeout.cc
SRCS+=	timer.cc

.if !defined(USE_POLL)
USE_POLL?=	kqueue
.endif

SRCS+=	event_poll_${USE_POLL}.cc

.if ${USE_POLL} == "kqueue"
CFLAGS+=-DUSE_POLL_KQUEUE
.elif ${USE_POLL} == "poll"
CFLAGS+=-DUSE_POLL_POLL
.else
.error "Unsupported poll mechanism."
.endif
