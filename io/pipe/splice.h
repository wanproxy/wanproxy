/*
 * Copyright (c) 2009-2013 Juli Mallett. All rights reserved.
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

#ifndef	IO_PIPE_SPLICE_H
#define	IO_PIPE_SPLICE_H

class CallbackScheduler;
class StreamChannel;
class Pipe;

class Splice {
	LogHandle log_;
	Mutex mtx_;
	CallbackScheduler *scheduler_;
	StreamChannel *source_;
	Pipe *pipe_;
	StreamChannel *sink_;

	EventCallback *callback_;
	Action *callback_action_;

	bool read_eos_;
	Action *read_action_;
	Action *input_action_;
	bool output_eos_;
	Action *output_action_;
	Action *write_action_;
	Action *shutdown_action_;

public:
	Splice(const LogHandle&, StreamChannel *, Pipe *, StreamChannel *);
	~Splice();

	Action *start(EventCallback *);

private:
	void cancel(void);
	void complete(Event);

	void read_complete(Event);
	void input_complete(Event);

	void output_complete(Event);
	void write_complete(Event);

	void shutdown_complete(Event);
};

#endif /* !IO_PIPE_SPLICE_H */
