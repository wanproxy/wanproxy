VPATH+=	${TOPDIR}/xcodec

SRCS+=	xcodec_decoder.cc
SRCS+=	xcodec_encoder.cc

CFLAGS_io+=-DXCODEC_PIPES
SRCS_io+=xcodec_decoder_pipe.cc
SRCS_io+=xcodec_encoder_pipe.cc
