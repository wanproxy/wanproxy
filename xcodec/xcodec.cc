#include <common/buffer.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_decoder.h>
#include <xcodec/xcodec_encoder.h>

XCodecDecoder *
XCodec::decoder(void)
{
	return (new XCodecDecoder(this));
}

XCodecEncoder *
XCodec::encoder(void)
{
	return (new XCodecEncoder(this));
}
