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
	XCodecEncoderPipe encoder_pipe_;
	XCodecDecoderPipe decoder_pipe_;
public:
	XCodecPipePair(XCodec *codec, XCodecPipePairType type)
	: type_(type),
	  encoder_pipe_(codec),
	  decoder_pipe_(codec)
	{
		decoder_pipe_.decoder_.set_encoder(&encoder_pipe_.encoder_);
		decoder_pipe_.decoder_.set_encoder_pipe(&encoder_pipe_);
	}

	~XCodecPipePair()
	{
		decoder_pipe_.decoder_.set_encoder_pipe(NULL);
		decoder_pipe_.decoder_.set_encoder(NULL);
	}

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
