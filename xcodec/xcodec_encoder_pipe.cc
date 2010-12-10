#include <event/event_callback.h>

#include <io/pipe/pipe.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_encoder.h>
#include <xcodec/xcodec_encoder_pipe.h>

XCodecEncoderPipe::XCodecEncoderPipe(XCodec *codec)
: PipeSimple("/xcodec/encoder/pipe"),
  encoder_(codec)
{
	encoder_.set_pipe(this);
}

XCodecEncoderPipe::~XCodecEncoderPipe()
{
	encoder_.set_pipe(NULL);
}

void
XCodecEncoderPipe::output_ready(void)
{
}

bool
XCodecEncoderPipe::process(Buffer *out, Buffer *in)
{
	encoder_.encode(out, in);
	return (true);
}
