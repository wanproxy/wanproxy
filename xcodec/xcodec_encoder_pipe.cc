#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe.h>

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
	PipeSimple::output_spontaneous();
}

bool
XCodecEncoderPipe::process(Buffer *out, Buffer *in)
{
	encoder_.encode(out, in);
	return (true);
}

/*
 * If there is no queued data, we have sent EOS and
 * we have received EOS, we can return EOS.
 */
bool
XCodecEncoderPipe::process_eos(void) const
{
	if (encoder_.sent_eos_ && encoder_.received_eos_ &&
	    encoder_.queued_.empty()) {
		return (true);
	}

	return (false);
}
