VPATH+=	${TOPDIR}/io/socket

SRCS+=	socket.cc
SRCS+=	socket_handle.cc
SRCS+=	socket_uinet.cc
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

LDADD+=		-L${TOPDIR}/network/uinet/lib/libuinet -luinet

