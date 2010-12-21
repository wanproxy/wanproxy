#ifndef	WANPROXY_CODEC_H
#define	WANPROXY_CODEC_H

class XCodec;

struct WANProxyCodec {
	std::string name_;
	XCodec *codec_;
	bool compressor_;

	WANProxyCodec(const std::string& name)
	: name_(name),
	  codec_(NULL),
	  compressor_(false)
	{ }
};

#endif /* !WANPROXY_CODEC_H */
