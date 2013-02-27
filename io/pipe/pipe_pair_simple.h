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

#ifndef	IO_PIPE_PIPE_PAIR_SIMPLE_H
#define	IO_PIPE_PIPE_PAIR_SIMPLE_H

#include <io/pipe/pipe_simple.h>
#include <io/pipe/pipe_simple_wrapper.h>

class PipePairSimple : public PipePair {
	PipeSimpleWrapper<PipePairSimple> *incoming_pipe_;
	PipeSimpleWrapper<PipePairSimple> *outgoing_pipe_;
protected:
	PipePairSimple(void)
	: incoming_pipe_(NULL),
	  outgoing_pipe_(NULL)
	{ }
public:
	virtual ~PipePairSimple()
	{
		if (incoming_pipe_ != NULL) {
			delete incoming_pipe_;
			incoming_pipe_ = NULL;
		}

		if (outgoing_pipe_ != NULL) {
			delete outgoing_pipe_;
			outgoing_pipe_ = NULL;
		}
	}

protected:
	virtual bool incoming_process(Buffer *, Buffer *) = 0;
	virtual bool outgoing_process(Buffer *, Buffer *) = 0;

public:
	Pipe *get_incoming(void)
	{
		ASSERT(log_, incoming_pipe_ == NULL);
		incoming_pipe_ = new PipeSimpleWrapper<PipePairSimple>(this, &PipePairSimple::incoming_process);
		return (incoming_pipe_);
	}

	Pipe *get_outgoing(void)
	{
		ASSERT(log_, outgoing_pipe_ == NULL);
		outgoing_pipe_ = new PipeSimpleWrapper<PipePairSimple>(this, &PipePairSimple::outgoing_process);
		return (outgoing_pipe_);
	}
};

#endif /* !IO_PIPE_PIPE_PAIR_SIMPLE_H */
