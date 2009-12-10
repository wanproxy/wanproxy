VPATH+=${TOPDIR}/common

SRCS+=	buffer.cc
SRCS+=	log.cc
SRCS+=	timer.cc

CFLAGS+=-include common/common.h
