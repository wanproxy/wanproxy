VPATH+=	${TOPDIR}/common

SRCS+=	buffer.cc
SRCS+=	log.cc
SRCS+=	timer.cc
SRCS+=	uuid.cc

CFLAGS+=-include common/common.h
