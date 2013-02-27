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

static Buffer zero_buffer;

class Source {
	LogHandle log_;

	StreamHandle fd_;
	Action *action_;
public:
	Source(int fd)
	: log_("/source"),
	  fd_(fd),
	  action_(NULL)
	{
		Buffer tmp(zero_buffer);
		EventCallback *cb = callback(this, &Source::write_complete);
		action_ = fd_.write(&tmp, cb);
	}

	~Source()
	{
		ASSERT(log_, action_ == NULL);
	}

	void write_complete(Event e)
	{
		action_->cancel();
		action_ = NULL;

		switch (e.type_) {
		case Event::Done:
		case Event::Error:
			break;
		default:
			HALT(log_) << "Unexpected event: " << e;
			return;
		}

		if (e.type_ == Event::Error) {
			SimpleCallback *cb = callback(this, &Source::close_complete);
			action_ = fd_.close(cb);
			return;
		}

		Buffer tmp(zero_buffer);
		EventCallback *cb = callback(this, &Source::write_complete);
		action_ = fd_.write(&tmp, cb);
	}

	void close_complete(void)
	{
		action_->cancel();
		action_ = NULL;
	}
};

int
main(void)
{
	static uint8_t zbuf[65536];
	memset(zbuf, 0, sizeof zbuf);

	zero_buffer.append(zbuf, sizeof zbuf);

	Source source(STDOUT_FILENO);

	event_main();
}
