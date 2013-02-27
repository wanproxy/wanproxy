/*
 * Copyright (c) 2009-2010 Juli Mallett. All rights reserved.
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

#include <common/test.h>

#include <event/event_callback.h>
#include <event/event_main.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_null.h>

static uint8_t data[65536];

class Producer {
	LogHandle log_;
	TestGroup group_;
	Pipe *pipe_;
	Action *action_;
public:
	Producer(Pipe *pipe)
	: log_("/producer"),
	  group_("/test/io/pipe/null/producer", "Producer"),
	  pipe_(pipe),
	  action_(NULL)
	{
		Buffer buf(data, sizeof data);

		EventCallback *cb = callback(this, &Producer::input_complete);
		action_ = pipe_->input(&buf, cb);

		{
			Test _(group_, "Input method consumed Buffer");
			if (buf.empty())
				_.pass();
		}
	}

	~Producer()
	{
		{
			Test _(group_, "No pending action");
			if (action_ == NULL)
				_.pass();
			else {
				action_->cancel();
				action_ = NULL;
			}
		}
	}

	void input_complete(Event e)
	{
		{
			Test _(group_, "Non-NULL Action");
			if (action_ != NULL) {
				action_->cancel();
				action_ = NULL;

				_.pass();
			}
		}

		{
			Test _(group_, "Expected event");
			if (e.type_ == Event::Done)
				_.pass();
		}
	}
};

class Consumer {
	LogHandle log_;
	TestGroup group_;
	Pipe *pipe_;
	Action *action_;
	Buffer buffer_;
public:
	Consumer(Pipe *pipe)
	: log_("/consumer"),
	  group_("/test/io/pipe/null/consumer", "Consumer"),
	  pipe_(pipe),
	  action_(NULL),
	  buffer_()
	{
		EventCallback *cb = callback(this, &Consumer::output_complete);
		action_ = pipe_->output(cb);
	}

	~Consumer()
	{
		{
			Test _(group_, "No pending action");
			if (action_ == NULL)
				_.pass();
			else {
				action_->cancel();
				action_ = NULL;
			}
		}

		{
			Test _(group_, "Got expected data");
			if (buffer_.equal(data, sizeof data))
				_.pass();
		}
	}

	void output_complete(Event e)
	{
		{
			Test _(group_, "Non-NULL Action");
			if (action_ != NULL) {
				action_->cancel();
				action_ = NULL;

				_.pass();
			}
		}

		{
			Test _(group_, "Expected event");
			if (e.type_ == Event::Done)
				_.pass();
		}

		{
			Test _(group_, "Non-empty Buffer");
			if (!e.buffer_.empty()) {
				buffer_.append(e.buffer_);

				_.pass();
			}
		}
	}
};

int
main(void)
{
	unsigned i;

	for (i = 0; i < sizeof data; i++)
		data[i] = random() % 0xff;

	PipeNull pipe;
	Producer producer(&pipe);
	Consumer consumer(&pipe);
	event_main();
}
