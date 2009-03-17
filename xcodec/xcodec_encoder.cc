#include <common/buffer.h>
#include <common/endian.h>

#include <xcodec/xcdb.h>
#include <xcodec/xchash.h>
#include <xcodec/xcodec.h>
#include <xcodec/xcodec_encoder.h>
#include <xcodec/xcodec_slice.h>

XCodecEncoder::XCodecEncoder(XCodec *codec)
: log_("/xcodec/encoder"),
  database_(codec->database_),
  backref_()
{ }

XCodecEncoder::~XCodecEncoder()
{ }

void
XCodecEncoder::encode(Buffer *output, Buffer *input)
{
	XCodecSlice slice(database_, input);
	ASSERT(input->empty());

	slice.encode(&backref_, output);
}
