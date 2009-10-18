.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.PATH: ${TOPDIR}/xcodec

SRCS+=	xcodec_decoder.cc
SRCS+=	xcodec_encoder.cc

.if ${USE_LIBS:Mio}
SRCS+=	xcodec_decoder_pipe.cc
SRCS+=	xcodec_encoder_pipe.cc
.endif
