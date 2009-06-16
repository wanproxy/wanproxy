PROG_CXX=wanproxy

SRCS+=	flow_monitor.cc

SRCS+=	proxy_client.cc
SRCS+=	proxy_listener.cc
SRCS+=	proxy_pipe.cc

SRCS+=	proxy_socks_connection.cc
SRCS+=	proxy_socks_listener.cc

SRCS+=	wanproxy.cc
SRCS+=	wanproxy_config.cc
SRCS+=	wanproxy_config_class_codec.cc
SRCS+=	wanproxy_config_class_interface.cc
SRCS+=	wanproxy_config_class_peer.cc
SRCS+=	wanproxy_config_class_proxy.cc
SRCS+=	wanproxy_config_class_proxy_socks.cc
SRCS+=	wanproxy_config_type_codec.c

TOPDIR=${.CURDIR}/../..
USE_LIBS=common config event io net xcodec
.include "${TOPDIR}/program.mk"
