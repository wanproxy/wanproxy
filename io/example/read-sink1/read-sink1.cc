/*
 * Copyright (c) 2010-2012 Juli Mallett. All rights reserved.
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

#include <event/callback_handler.h>
#include <event/event_callback.h>
#include <event/event_handler.h>
#include <event/event_main.h>

#include <io/stream_handle.h>

class Sink {
	LogHandle log_;

	StreamHandle fd_;
	CallbackHandler close_handler_;
	EventHandler read_handler_;
public:
	Sink(int fd)
	: log_("/sink"),
	  fd_(fd),
	  read_handler_()
	{
		read_handler_.handler(Event::Done, this, &Sink::read_done);
		read_handler_.handler(Event::EOS, this, &Sink::read_eos);

		close_handler_.handler(this, &Sink::close_done);

		read_handler_.wait(fd_.read(0, read_handler_.callback()));
	}

	~Sink()
	{ }

	void read_done(Event)
	{
		read_handler_.wait(fd_.read(0, read_handler_.callback()));
	}

	void read_eos(Event)
	{
		close_handler_.wait(fd_.close(close_handler_.callback()));
	}

	void close_done(void)
	{ }
};

int
main(void)
{
	Sink sink(STDIN_FILENO);

	event_main();
}
