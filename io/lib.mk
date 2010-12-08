VPATH+=	${TOPDIR}/io

SRCS+=	block_device.cc
SRCS+=	file_descriptor.cc
SRCS+=	io_system.cc
SRCS+=	socket.cc
SRCS+=	unix_client.cc
SRCS+=	unix_server.cc

ifeq "${OSNAME}" "Haiku"
# Required for sockets.
LDADD+=		-lnetwork
endif

ifeq "${OSNAME}" "SunOS"
# Required for sockets.
LDADD+=		-lnsl
LDADD+=		-lsocket
endif
