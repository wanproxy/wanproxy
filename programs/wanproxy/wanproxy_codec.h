#ifndef	WANPROXY_CODEC_H
#define	WANPROXY_CODEC_H

class XCodec;

struct WANProxyCodec {
	std::string name_;
	XCodec *codec_;
	bool compressor_;
	unsigned compressor_level_;

	WANProxyCodec(const std::string& name)
	: name_(name),
	  codec_(NULL),
	  compressor_(false),
	  compressor_level_(0)
	{ }
};

#endif /* !WANPROXY_CODEC_H */
