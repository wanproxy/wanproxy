#include <common/buffer.h>
#include <common/endian.h>

#include <xcodec/xcdb.h>
#include <xcodec/xchash.h>
#include <xcodec/xcodec.h>
#include <xcodec/xcodec_encoder.h>

struct xcodec_special_p {
	bool operator() (uint8_t ch) const
	{
		return (XCODEC_CHAR_SPECIAL(ch));
	}
};

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
	input->escape(XCODEC_ESCAPE_CHAR, xcodec_special_p());
	output->append(input);
	input->clear();
}
