VPATH+=	${TOPDIR}/xcodec

SRCS+=	xcodec_decoder.cc
SRCS+=	xcodec_encoder.cc

define __per_library
_lib:=$(1)
ifeq "${_lib}" "io"
CFLAGS+=-DXCODEC_PIPES
SRCS+=	xcodec_decoder_pipe.cc
SRCS+=	xcodec_encoder_pipe.cc
endif
endef

$(foreach _lib, ${USE_LIBS}, $(eval $(call __per_library, ${_lib})))
