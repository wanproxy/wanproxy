#ifndef	PROGRAMS_WANPROXY_WANPROXY_CODEC_H
#define	PROGRAMS_WANPROXY_WANPROXY_CODEC_H

class XCodec;

struct WANProxyCodec {
	std::string name_;
	XCodec *codec_;
	bool compressor_;
	unsigned compressor_level_;

	intmax_t *outgoing_to_codec_bytes_;
	intmax_t *codec_to_outgoing_bytes_;
	intmax_t *incoming_to_codec_bytes_;
	intmax_t *codec_to_incoming_bytes_;

	WANProxyCodec(void)
	: name_(""),
	  codec_(NULL),
	  compressor_(false),
	  compressor_level_(0),
	  outgoing_to_codec_bytes_(NULL),
	  codec_to_outgoing_bytes_(NULL),
	  incoming_to_codec_bytes_(NULL),
	  codec_to_incoming_bytes_(NULL)
	{ }
};

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CODEC_H */
