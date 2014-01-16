VPATH+=	${TOPDIR}/io/socket

SRCS+=	resolver.cc
SRCS+=	socket.cc
SRCS+=	socket_handle.cc
SRCS+=	unix_client.cc
SRCS+=	unix_server.cc

SRCS_network_uinet+=	socket_uinet.cc
SRCS_network_uinet+=	socket_uinet_promisc.cc

ifeq "${OSNAME}" "Haiku"
# Required for sockets.
LDADD+=		-lnetwork
endif

ifeq "${OSNAME}" "SunOS"
# Required for sockets.
LDADD+=		-lnsl
LDADD+=		-lsocket
endif

