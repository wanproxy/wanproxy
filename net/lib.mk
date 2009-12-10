VPATH+=${TOPDIR}/net

SRCS+=	tcp_client.cc
SRCS+=	tcp_server.cc
SRCS+=	udp_client.cc
SRCS+=	udp_server.cc

NET_REQUIRES=	event io
