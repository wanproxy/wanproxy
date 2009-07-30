.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.PATH: ${TOPDIR}/net

SRCS+=	tcp_client.cc
SRCS+=	tcp_server.cc
SRCS+=	udp_client.cc

NET_REQUIRES=	event io
