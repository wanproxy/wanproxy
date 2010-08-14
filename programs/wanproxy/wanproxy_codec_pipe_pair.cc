#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <io/pipe.h>
#include <io/pipe_link.h>
#include <io/pipe_null.h>
#include <io/pipe_pair.h>

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
  pipe_pairs_()
{
	std::deque<std::pair<Pipe *, Pipe *> > pipe_list;

	if (incoming != NULL) {
		if (incoming->compressor_) {
			std::pair<Pipe *, Pipe *> pipe_pair(new InflatePipe(), new DeflatePipe(9));

			pipes_.insert(pipe_pair.first);
			pipes_.insert(pipe_pair.second);

			pipe_list.push_back(pipe_pair);
		}

		if (incoming->codec_ != NULL) {
			PipePair *pair = new XCodecPipePair(incoming->codec_, XCodecPipePairTypeServer);
			pipe_pairs_.insert(pair);

			std::pair<Pipe *, Pipe *> pipe_pair(pair->get_incoming(), pair->get_outgoing());
			pipe_list.push_back(pipe_pair);
		}
	}

	if (outgoing != NULL) {
		if (outgoing->codec_ != NULL) {
			PipePair *pair = new XCodecPipePair(outgoing->codec_, XCodecPipePairTypeClient);
			pipe_pairs_.insert(pair);

			std::pair<Pipe *, Pipe *> pipe_pair(pair->get_incoming(), pair->get_outgoing());
			pipe_list.push_back(pipe_pair);
		}

		if (outgoing->compressor_) {
			std::pair<Pipe *, Pipe *> pipe_pair(new DeflatePipe(9), new InflatePipe());

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

	incoming_pipe_ = pipe_list.front().first;
	outgoing_pipe_ = pipe_list.front().second;
	pipe_list.pop_front();

	while (!pipe_list.empty()) {
		incoming_pipe_ = new PipeLink(incoming_pipe_, pipe_list.front().first);
		outgoing_pipe_ = new PipeLink(outgoing_pipe_, pipe_list.front().second);

		pipes_.insert(incoming_pipe_);
		pipes_.insert(outgoing_pipe_);

		pipe_list.pop_front();
	}
}

WANProxyCodecPipePair::~WANProxyCodecPipePair()
{
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
