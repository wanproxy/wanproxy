/*
 * Copyright (c) 2010-2013 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <common/endian.h>
#include <common/thread/mutex.h>

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

namespace {
	class PipeByteCount : public PipeProducer {
		Mutex mtx_;
		intmax_t *counterp_;
	public:
		PipeByteCount(intmax_t *counterp)
		: PipeProducer("/wanproxy/codec/byte_count", &mtx_),
		  mtx_("PipeByteCount"),
		  counterp_(counterp)
		{ }

		~PipeByteCount()
		{ }

	private:
		void consume(Buffer *buf)
		{
			ASSERT_LOCK_OWNED(log_, &mtx_);
			if (!buf->empty()) {
				if (counterp_ != NULL)
					*counterp_ += buf->length();
				produce(buf);
			} else {
				produce_eos();
			}
		}
	};
};

WANProxyCodecPipePair::WANProxyCodecPipePair(WANProxyCodec *incoming, WANProxyCodec *outgoing)
: incoming_pipe_(NULL),
  outgoing_pipe_(NULL),
  pipes_(),
  pipe_pairs_(),
  pipe_links_()
{
	std::deque<Pipe *> incoming_pipe_list, outgoing_pipe_list;

	if (incoming != NULL) {
		if (incoming->track_statistics_) {
			Pipe *incoming_pipe = new PipeByteCount(incoming->incoming_to_codec_bytes_);
			Pipe *outgoing_pipe = new PipeByteCount(incoming->codec_to_incoming_bytes_);

			incoming_pipe_list.push_back(incoming_pipe);
			outgoing_pipe_list.push_front(outgoing_pipe);

			pipes_.insert(incoming_pipe);
			pipes_.insert(outgoing_pipe);
		}

		if (incoming->compressor_) {
			Pipe *deflate_pipe = new DeflatePipe(incoming->compressor_level_);
			Pipe *inflate_pipe = new InflatePipe();

			incoming_pipe_list.push_back(inflate_pipe);
			outgoing_pipe_list.push_front(deflate_pipe);

			pipes_.insert(deflate_pipe);
			pipes_.insert(inflate_pipe);
		}

		if (incoming->codec_ != NULL) {
			PipePair *pair = new XCodecPipePair("/wanproxy/codec/" + incoming->name_, incoming->codec_, XCodecPipePairTypeServer);
			pipe_pairs_.insert(pair);

			incoming_pipe_list.push_back(pair->get_incoming());
			outgoing_pipe_list.push_front(pair->get_outgoing());
		}

		if (incoming->track_statistics_) {
			Pipe *incoming_pipe = new PipeByteCount(incoming->codec_to_outgoing_bytes_);
			Pipe *outgoing_pipe = new PipeByteCount(incoming->outgoing_to_codec_bytes_);

			incoming_pipe_list.push_back(incoming_pipe);
			outgoing_pipe_list.push_front(outgoing_pipe);

			pipes_.insert(incoming_pipe);
			pipes_.insert(outgoing_pipe);
		}
	}

	if (outgoing != NULL) {
		if (outgoing->track_statistics_) {
			Pipe *incoming_pipe = new PipeByteCount(outgoing->incoming_to_codec_bytes_);
			Pipe *outgoing_pipe = new PipeByteCount(outgoing->codec_to_incoming_bytes_);

			incoming_pipe_list.push_back(incoming_pipe);
			outgoing_pipe_list.push_front(outgoing_pipe);

			pipes_.insert(incoming_pipe);
			pipes_.insert(outgoing_pipe);
		}

		if (outgoing->codec_ != NULL) {
			PipePair *pair = new XCodecPipePair("/wanproxy/codec/" + outgoing->name_, outgoing->codec_, XCodecPipePairTypeClient);
			pipe_pairs_.insert(pair);

			incoming_pipe_list.push_back(pair->get_incoming());
			outgoing_pipe_list.push_front(pair->get_outgoing());
		}

		if (outgoing->compressor_) {
			Pipe *deflate_pipe = new DeflatePipe(outgoing->compressor_level_);
			Pipe *inflate_pipe = new InflatePipe();

			incoming_pipe_list.push_back(deflate_pipe);
			outgoing_pipe_list.push_front(inflate_pipe);

			pipes_.insert(deflate_pipe);
			pipes_.insert(inflate_pipe);
		}

		if (outgoing->track_statistics_) {
			Pipe *incoming_pipe = new PipeByteCount(outgoing->codec_to_outgoing_bytes_);
			Pipe *outgoing_pipe = new PipeByteCount(outgoing->outgoing_to_codec_bytes_);

			incoming_pipe_list.push_back(incoming_pipe);
			outgoing_pipe_list.push_front(outgoing_pipe);

			pipes_.insert(incoming_pipe);
			pipes_.insert(outgoing_pipe);
		}
	}

	ASSERT("/wanproxy/codec/pipe/pair/config", incoming_pipe_list.empty() == outgoing_pipe_list.empty());

	if (incoming_pipe_list.empty() && outgoing_pipe_list.empty()) {
		return;
	}

	ASSERT("/wanproxy/codec/pipe/pair/config", incoming_pipe_list.size() == outgoing_pipe_list.size());

	incoming_pipe_ = incoming_pipe_list.front();
	incoming_pipe_list.pop_front();

	outgoing_pipe_ = outgoing_pipe_list.front();
	outgoing_pipe_list.pop_front();

	while (!incoming_pipe_list.empty() && !outgoing_pipe_list.empty()) {
		incoming_pipe_ = new PipeLink(incoming_pipe_, incoming_pipe_list.front());
		outgoing_pipe_ = new PipeLink(outgoing_pipe_, outgoing_pipe_list.front());

		pipe_links_.push_front(incoming_pipe_);
		pipe_links_.push_front(outgoing_pipe_);

		incoming_pipe_list.pop_front();
		outgoing_pipe_list.pop_front();
	}
	ASSERT("/wanproxy/codec/pipe/pair/config", incoming_pipe_list.empty() && outgoing_pipe_list.empty());
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
