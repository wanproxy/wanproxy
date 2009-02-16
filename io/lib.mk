.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.PATH: ${TOPDIR}/io

SRCS+=	file.cc
SRCS+=	file_descriptor.cc
SRCS+=	serial.cc
SRCS+=	socket.cc
SRCS+=	tcp_client.cc
SRCS+=	tcp_server.cc
