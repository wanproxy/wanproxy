.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.PATH: ${TOPDIR}/common

SRCS+=	buffer.cc
SRCS+=	log.cc

CFLAGS+=-include common/common.h
