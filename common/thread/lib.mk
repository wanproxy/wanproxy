VPATH+=	${TOPDIR}/common/thread

THREAD_MODEL=	posix

SRCS+=	mutex_${THREAD_MODEL}.cc
SRCS+=	sleep_queue_${THREAD_MODEL}.cc
SRCS+=	thread_${THREAD_MODEL}.cc

LDADD+=	-lpthread
