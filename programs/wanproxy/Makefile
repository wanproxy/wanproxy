PROG_CXX=wanproxy
# XXX Uncomment below for Linux.
#PROG=${PROG_CXX}
#USE_POLL=poll
#LDADD+=-lstdc++

SRCS+=	proxy_client.cc
SRCS+=	proxy_listener.cc
SRCS+=	proxy_peer.cc

SRCS+=	proxy_socks_connection.cc
SRCS+=	proxy_socks_listener.cc

SRCS+=	wanproxy.cc
SRCS+=	wanproxy_config.cc

TOPDIR=${.CURDIR}/../..
USE_LIBS=common event io xcodec
.include "${TOPDIR}/program.mk"
