.if !defined(TOPDIR)
.error "TOPDIR must be defined."
.endif

.PATH: ${TOPDIR}/xcodec

SRCS+=	xcodec.cc
SRCS+=	xcodec_decoder.cc
SRCS+=	xcodec_encoder.cc

.if ${USE_LIBS:Mevent}
SRCS+=	xcodec_encoder_stats.cc
CFLAGS+=-DXCODEC_STATS
.endif

.if ${USE_LIBS:Mio}
SRCS+=	xcodec_decoder_pipe.cc
SRCS+=	xcodec_encoder_pipe.cc
.endif
