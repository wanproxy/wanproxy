/*
 * Copyright (c) 2010-2011 Juli Mallett. All rights reserved.
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
