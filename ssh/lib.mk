VPATH+=	${TOPDIR}/ssh

SRCS+=	ssh_algorithm_negotiation.cc
SRCS+=	ssh_transport_pipe.cc

SRCS+=	ssh_compression.cc
SRCS+=	ssh_encryption.cc
SRCS+=	ssh_key_exchange.cc
SRCS+=	ssh_mac.cc
SRCS+=	ssh_server_host_key.cc
