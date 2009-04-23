.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.PATH: ${TOPDIR}/zzcodec

SRCS+=	zzcodec.cc
