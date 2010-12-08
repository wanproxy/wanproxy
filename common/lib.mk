VPATH+=	${TOPDIR}/common

SRCS+=	buffer.cc
SRCS+=	log.cc

CXXFLAGS+=-include common/common.h
