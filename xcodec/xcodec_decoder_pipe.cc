#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_decoder.h>
#include <xcodec/xcodec_decoder_pipe.h>

XCodecDecoderPipe::XCodecDecoderPipe(XCodec *codec)
: PipeSimple("/xcodec/decoder/pipe"),
  decoder_(codec)
{ }

XCodecDecoderPipe::~XCodecDecoderPipe()
{ }

bool
XCodecDecoderPipe::process(Buffer *out, Buffer *in)
{
	return (decoder_.decode(out, in));
}
