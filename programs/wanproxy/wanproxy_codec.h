#ifndef	PROGRAMS_WANPROXY_WANPROXY_CODEC_H
#define	PROGRAMS_WANPROXY_WANPROXY_CODEC_H

class XCodec;

struct WANProxyCodec {
	std::string name_;
	XCodec *codec_;
	bool compressor_;
	unsigned compressor_level_;

	WANProxyCodec(void)
	: name_(""),
	  codec_(NULL),
	  compressor_(false),
	  compressor_level_(0)
	{ }
};

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CODEC_H */
