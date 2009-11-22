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
  encoder_(codec),
  sent_eos_(false)
{
	encoder_.set_pipe(this);
}

XCodecEncoderPipe::~XCodecEncoderPipe()
{
	encoder_.set_pipe(NULL);
}

void
XCodecEncoderPipe::received_eos(bool eos)
{
	received_eos_ = eos;
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
	if (out->empty() && in->empty()) {
		if (input_eos_) {
			if (sent_eos_ && received_eos_) {
				return (true);
			}
			out->append(XCODEC_MAGIC);
			out->append(XCODEC_OP_EOS);
			sent_eos_ = true;
		}
	} else {
		sent_eos_ = false;
	}
	return (true);
}
