#ifndef	XCODEC_PIPE_PAIR_H
#define	XCODEC_PIPE_PAIR_H

#include <xcodec/xcodec_decoder_pipe.h>
#include <xcodec/xcodec_encoder_pipe.h>

enum XCodecPipePairType {
	XCodecPipePairTypeClient,
	XCodecPipePairTypeServer,
};

class XCodecPipePair : public PipePair {
	XCodecPipePairType type_;
	XCodecDecoderPipe decoder_pipe_;
	XCodecEncoderPipe encoder_pipe_;
public:
	PipePairEcho(XCodec *codec, XCodecPipePairType type)
	: type_(type),
	  decoder_pipe_(codec),
	  encoder_pipe_(codec)
	{ }

	~PipePairEcho()
	{ }

	Pipe *get_incoming(void)
	{
		switch (type_) {
		case XCodecPipePairTypeClient:
			return (&decoder_pipe_);
		case XCodecPipePairTypeServer:
			return (&encoder_pipe_);
		default:
			NOTREACHED();
		}
	}

	Pipe *get_outgoing(void)
	{
		switch (type_) {
		case XCodecPipePairTypeClient:
			return (&encoder_pipe_);
		case XCodecPipePairTypeServer:
			return (&decoder_pipe_);
		default:
			NOTREACHED();
		}
	}
};

#endif /* !XCODEC_PIPE_PAIR_H */
