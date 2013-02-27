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

#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/stream_handle.h>
#include <io/pipe/pipe.h>
#include <io/pipe/splice.h>

#include <zlib/inflate_pipe.h>

class Catenate {
	LogHandle log_;

	StreamHandle input_;
	Action *input_action_;

	StreamHandle output_;
	Action *output_action_;

	Splice splice_;
	Action *splice_action_;
public:
	Catenate(int input, Pipe *pipe, int output)
	: log_("/catenate"),
	  input_(input),
	  input_action_(NULL),
	  output_(output),
	  output_action_(NULL),
	  splice_(log_, &input_, pipe, &output_)
	{
		EventCallback *cb = callback(this, &Catenate::splice_complete);
		splice_action_ = splice_.start(cb);
	}

	~Catenate()
	{
		ASSERT(log_, input_action_ == NULL);
		ASSERT(log_, output_action_ == NULL);
		ASSERT(log_, splice_action_ == NULL);
	}

	void splice_complete(Event e)
	{
		splice_action_->cancel();
		splice_action_ = NULL;

		switch (e.type_) {
		case Event::EOS:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		SimpleCallback *icb = callback(this, &Catenate::close_complete, &input_);
		input_action_ = input_.close(icb);

		SimpleCallback *ocb = callback(this, &Catenate::close_complete, &output_);
		output_action_ = output_.close(ocb);
	}

	void close_complete(StreamHandle *fd)
	{
		if (fd == &input_) {
			input_action_->cancel();
			input_action_ = NULL;
		} else if (fd == &output_) {
			output_action_->cancel();
			output_action_ = NULL;
		} else {
			NOTREACHED(log_);
		}
	}
};

int
main(void)
{
	InflatePipe pipe;
	Catenate cat(STDIN_FILENO, &pipe, STDOUT_FILENO);

	event_main();
}
