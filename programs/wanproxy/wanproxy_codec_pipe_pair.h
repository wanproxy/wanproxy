#ifndef	WANPROXY_CODEC_PIPE_H
#define	WANPROXY_CODEC_PIPE_H

#include <set>

#include <io/pipe/pipe_pair.h>

struct WANProxyCodec;
class XCodec;

class WANProxyCodecPipePair : public PipePair {
	Pipe *incoming_pipe_;
	Pipe *outgoing_pipe_;

	std::set<Pipe *> pipes_;
	std::set<PipePair *> pipe_pairs_;
public:
	WANProxyCodecPipePair(WANProxyCodec *, WANProxyCodec *);
	~WANProxyCodecPipePair();

	Pipe *get_incoming(void);
	Pipe *get_outgoing(void);
};

#endif /* !WANPROXY_CODEC_PIPE_H */
