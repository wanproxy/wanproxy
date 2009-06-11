PROG_CXX=wanproxy

SRCS+=	flow_monitor.cc

SRCS+=	proxy_client.cc
SRCS+=	proxy_listener.cc
SRCS+=	proxy_pipe.cc

SRCS+=	proxy_socks_connection.cc
SRCS+=	proxy_socks_listener.cc

SRCS+=	wanproxy.cc
SRCS+=	wanproxy_config.cc

TOPDIR=${.CURDIR}/../..
USE_LIBS=common config event io net xcodec
.include "${TOPDIR}/program.mk"
