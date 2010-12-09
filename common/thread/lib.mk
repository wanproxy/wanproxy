VPATH+=	${TOPDIR}/common/thread

SRCS+=	thread_posix.cc

LDADD+=	-lpthread
