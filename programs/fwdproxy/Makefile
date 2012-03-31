PROGRAM=fwdproxy

SRCS+=	proxy_connector.cc
SRCS+=	proxy_listener.cc

SRCS+=	fwdproxy.cc
SRCS+=	fwdproxy_config.cc

TOPDIR=../..
USE_LIBS=common common/thread common/time event io io/net io/pipe io/socket
include ${TOPDIR}/common/program.mk
