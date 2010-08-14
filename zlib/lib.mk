VPATH+=	${TOPDIR}/zlib

SRCS+=	deflate_pipe.cc

LDADD+=	-lz
