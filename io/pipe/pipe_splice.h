/*
 * Copyright (c) 2010-2016 Juli Mallett. All rights reserved.
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

#ifndef	IO_PIPE_PIPE_SPLICE_H
#define	IO_PIPE_PIPE_SPLICE_H

#include <event/cancellation.h>

class PipeSplice {
	LogHandle log_;
	Mutex mtx_;
	Pipe *source_;
	Pipe *sink_;
	bool source_eos_;
	EventCallback::Method<PipeSplice> output_complete_;
	EventCallback::Method<PipeSplice> input_complete_;
	Cancellation<PipeSplice> cancel_;
	Action *action_;
	EventCallback *callback_;
public:
	PipeSplice(Pipe *, Pipe *);
	~PipeSplice();

	Action *start(EventCallback *cb);
private:
	void cancel(void);

	void output_complete(Event);
	void input_complete(Event);
};

#endif /* !IO_PIPE_PIPE_SPLICE_H */
