#ifndef	PROGRAMS_WANPROXY_WANPROXY_CODEC_PIPE_PAIR_H
#define	PROGRAMS_WANPROXY_WANPROXY_CODEC_PIPE_PAIR_H

#include <list>
#include <set>

#include <io/pipe/pipe_pair.h>

struct WANProxyCodec;
class XCodec;

class WANProxyCodecPipePair : public PipePair {
	Pipe *incoming_pipe_;
	Pipe *outgoing_pipe_;

	std::set<Pipe *> pipes_;
	std::set<PipePair *> pipe_pairs_;
	std::list<Pipe *> pipe_links_;
public:
	WANProxyCodecPipePair(WANProxyCodec *, WANProxyCodec *);
	~WANProxyCodecPipePair();

	Pipe *get_incoming(void);
	Pipe *get_outgoing(void);
};

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CODEC_PIPE_PAIR_H */
