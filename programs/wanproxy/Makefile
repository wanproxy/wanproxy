PROGRAM=wanproxy

SRCS+=	monitor_client.cc

SRCS+=	proxy_connector.cc
SRCS+=	proxy_listener.cc

SRCS+=	proxy_socks_connection.cc
SRCS+=	proxy_socks_listener.cc

SRCS+=	ssh_proxy_connector.cc
SRCS+=	ssh_proxy_listener.cc
SRCS+=	ssh_stream.cc

SRCS+=	wanproxy.cc
SRCS+=	wanproxy_codec_pipe_pair.cc
SRCS+=	wanproxy_config.cc
SRCS+=	wanproxy_config_class_codec.cc
SRCS+=	wanproxy_config_class_interface.cc
SRCS+=	wanproxy_config_class_peer.cc
SRCS+=	wanproxy_config_class_proxy.cc
SRCS+=	wanproxy_config_class_proxy_socks.cc
SRCS+=	wanproxy_config_class_monitor.cc
SRCS+=	wanproxy_config_type_codec.cc
SRCS+=	wanproxy_config_type_compressor.cc
SRCS+=	wanproxy_config_type_proxy_type.cc

TOPDIR=../..
USE_LIBS=common common/thread common/time common/uuid config crypto event http io io/net io/pipe io/socket ssh xcodec zlib
include ${TOPDIR}/common/program.mk
