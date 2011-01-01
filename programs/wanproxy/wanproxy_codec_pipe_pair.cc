#include <common/endian.h>

#include <event/event_callback.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_link.h>
#include <io/pipe/pipe_null.h>
#include <io/pipe/pipe_pair.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_decoder.h>
#include <xcodec/xcodec_encoder.h>
#include <xcodec/xcodec_pipe_pair.h>

#include <zlib/deflate_pipe.h>
#include <zlib/inflate_pipe.h>

#include "wanproxy_codec.h"
#include "wanproxy_codec_pipe_pair.h"

WANProxyCodecPipePair::WANProxyCodecPipePair(WANProxyCodec *incoming, WANProxyCodec *outgoing)
: incoming_pipe_(NULL),
  outgoing_pipe_(NULL),
  pipes_(),
  pipe_pairs_(),
  pipe_links_()
{
	std::deque<std::pair<Pipe *, Pipe *> > pipe_list;

	if (incoming != NULL) {
		if (incoming->compressor_) {
			std::pair<Pipe *, Pipe *> pipe_pair(new DeflatePipe(incoming->compressor_level_), new InflatePipe());

			pipes_.insert(pipe_pair.first);
			pipes_.insert(pipe_pair.second);

			pipe_list.push_back(pipe_pair);
		}

		if (incoming->codec_ != NULL) {
			PipePair *pair = new XCodecPipePair("/wanproxy/codec/" + incoming->name_, incoming->codec_, XCodecPipePairTypeServer);
			pipe_pairs_.insert(pair);

			std::pair<Pipe *, Pipe *> pipe_pair(pair->get_incoming(), pair->get_outgoing());
			pipe_list.push_back(pipe_pair);
		}
	}

	if (outgoing != NULL) {
		if (outgoing->codec_ != NULL) {
			PipePair *pair = new XCodecPipePair("/wanproxy/codec/" + outgoing->name_, outgoing->codec_, XCodecPipePairTypeClient);
			pipe_pairs_.insert(pair);

			std::pair<Pipe *, Pipe *> pipe_pair(pair->get_incoming(), pair->get_outgoing());
			pipe_list.push_back(pipe_pair);
		}

		if (outgoing->compressor_) {
			std::pair<Pipe *, Pipe *> pipe_pair(new InflatePipe(), new DeflatePipe(outgoing->compressor_level_));

			pipes_.insert(pipe_pair.first);
			pipes_.insert(pipe_pair.second);

			pipe_list.push_back(pipe_pair);
		}
	}

	if (pipe_list.empty()) {
		incoming_pipe_ = new PipeNull();
		outgoing_pipe_ = new PipeNull();

		pipes_.insert(incoming_pipe_);
		pipes_.insert(outgoing_pipe_);

		return;
	}

	/*
	 * XXX
	 * I think the incoming Pipes may be in the wrong order.
	 */

	incoming_pipe_ = pipe_list.front().first;
	outgoing_pipe_ = pipe_list.front().second;
	pipe_list.pop_front();

	while (!pipe_list.empty()) {
		incoming_pipe_ = new PipeLink(incoming_pipe_, pipe_list.front().first);
		outgoing_pipe_ = new PipeLink(outgoing_pipe_, pipe_list.front().second);

		pipe_links_.push_front(incoming_pipe_);
		pipe_links_.push_front(outgoing_pipe_);

		pipe_list.pop_front();
	}
}

WANProxyCodecPipePair::~WANProxyCodecPipePair()
{
	/*
	 * We need to delete the PipeLinks first, since they may have
	 * actions running internally.
	 */
	std::list<Pipe *>::iterator plit;
	while ((plit = pipe_links_.begin()) != pipe_links_.end()) {
		Pipe *pipe_link = *plit;
		pipe_links_.erase(plit);

		delete pipe_link;
	}

	std::set<Pipe *>::iterator pit;
	while ((pit = pipes_.begin()) != pipes_.end()) {
		Pipe *pipe = *pit;
		pipes_.erase(pit);

		delete pipe;
	}

	std::set<PipePair *>::iterator ppit;
	while ((ppit = pipe_pairs_.begin()) != pipe_pairs_.end()) {
		PipePair *pipe_pair = *ppit;
		pipe_pairs_.erase(ppit);

		delete pipe_pair;
	}
}

Pipe *
WANProxyCodecPipePair::get_incoming(void)
{
	return (incoming_pipe_);
}

Pipe *
WANProxyCodecPipePair::get_outgoing(void)
{
	return (outgoing_pipe_);
}
