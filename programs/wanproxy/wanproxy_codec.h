#ifndef	WANPROXY_CODEC_H
#define	WANPROXY_CODEC_H

class XCodec;

struct WANProxyCodec {
	XCodec *codec_;
	bool compressor_;
};

#endif /* !WANPROXY_CODEC_H */
