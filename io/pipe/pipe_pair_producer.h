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

#ifndef	IO_PIPE_PIPE_PAIR_PRODUCER_H
#define	IO_PIPE_PIPE_PAIR_PRODUCER_H

#include <io/pipe/pipe_producer.h>
#include <io/pipe/pipe_producer_wrapper.h>

class PipePairProducer : public PipePair {
	PipeProducerWrapper<PipePairProducer> *incoming_pipe_;
	PipeProducerWrapper<PipePairProducer> *outgoing_pipe_;
protected:
	PipePairProducer(const LogHandle& log, Lock *lock)
	: incoming_pipe_(NULL),
	  outgoing_pipe_(NULL)
	{
		incoming_pipe_ = new PipeProducerWrapper<PipePairProducer>(log + "/incoming", lock, this, &PipePairProducer::incoming_consume);
		outgoing_pipe_ = new PipeProducerWrapper<PipePairProducer>(log + "/outgoing", lock, this, &PipePairProducer::outgoing_consume);
	}
public:
	virtual ~PipePairProducer()
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
	virtual void incoming_consume(Buffer *) = 0;
	virtual void outgoing_consume(Buffer *) = 0;

	void incoming_produce(Buffer *buf)
	{
		incoming_pipe_->produce(buf);
	}

	void incoming_produce_eos(Buffer *buf = NULL)
	{
		incoming_pipe_->produce_eos(buf);
	}

	void incoming_produce_error(void)
	{
		incoming_pipe_->produce_error();
	}

	void outgoing_produce(Buffer *buf)
	{
		outgoing_pipe_->produce(buf);
	}

	void outgoing_produce_eos(Buffer *buf = NULL)
	{
		outgoing_pipe_->produce_eos(buf);
	}

	void outgoing_produce_error(void)
	{
		outgoing_pipe_->produce_error();
	}

public:
	Pipe *get_incoming(void)
	{
		return (incoming_pipe_);
	}

	Pipe *get_outgoing(void)
	{
		return (outgoing_pipe_);
	}
};

#endif /* !IO_PIPE_PIPE_PAIR_PRODUCER_H */
