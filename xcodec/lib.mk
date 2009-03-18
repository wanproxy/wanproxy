.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.PATH: ${TOPDIR}/xcodec

SRCS+=	xcodec.cc
SRCS+=	xcodec_decoder.cc
SRCS+=	xcodec_encoder.cc
