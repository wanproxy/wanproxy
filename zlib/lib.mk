VPATH+=	${TOPDIR}/zlib

SRCS+=	deflate_pipe.cc
SRCS+=	inflate_pipe.cc

LDADD+=	-lz
