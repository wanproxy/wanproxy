VPATH+=	${TOPDIR}/common/uuid

ifeq "${OSNAME}" "FreeBSD"
SRCS+=	uuid_libc.cc
else
SRCS+=	uuid_libuuid.cc
endif

ifeq "${OSNAME}" "Linux"
LDADD+=	-luuid
endif
