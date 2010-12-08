VPATH+=	${TOPDIR}/io/socket

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
