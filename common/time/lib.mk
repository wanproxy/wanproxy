VPATH+=	${TOPDIR}/common/time

ifeq "${OSNAME}" "Linux"
# Required for clock_gettime.
LDADD+=         -lrt
endif


SRCS+=	time.cc
