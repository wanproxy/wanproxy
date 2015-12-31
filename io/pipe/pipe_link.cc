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

#include <common/thread/mutex.h>

#include <event/event_callback.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_link.h>
#include <io/pipe/pipe_splice.h>

/*
 * PipeLink is a pipe which passes data through two Pipes.
 */

PipeLink::PipeLink(Pipe *incoming_pipe, Pipe *outgoing_pipe)
: log_("/pipe/link"),
  mtx_("PipeLink"),
  incoming_pipe_(incoming_pipe),
  outgoing_pipe_(outgoing_pipe),
  pipe_splice_(NULL),
  pipe_splice_action_(NULL),
  pipe_splice_error_(false)
{
	ScopedLock _(&mtx_);
	pipe_splice_ = new PipeSplice(incoming_pipe_, outgoing_pipe_);
	EventCallback *cb = callback(&mtx_, this, &PipeLink::pipe_splice_complete);
	pipe_splice_action_ = pipe_splice_->start(cb);
}

PipeLink::~PipeLink()
{
	ScopedLock _(&mtx_);
	if (pipe_splice_ != NULL) {
		ASSERT_NON_NULL(log_, pipe_splice_action_);
		pipe_splice_action_->cancel();
		pipe_splice_action_ = NULL;

		delete pipe_splice_;
		pipe_splice_ = NULL;
	} else {
		ASSERT_NULL(log_, pipe_splice_action_);
	}
}

Action *
PipeLink::input(Buffer *buf, EventCallback *cb)
{
	ScopedLock _(&mtx_);
	if (pipe_splice_error_) {
		buf->clear();
		cb->param(Event::Error);
		return (cb->schedule());
	}
	return (incoming_pipe_->input(buf, cb));
}

Action *
PipeLink::output(EventCallback *cb)
{
	ScopedLock _(&mtx_);
	if (pipe_splice_error_) {
		cb->param(Event::Error);
		return (cb->schedule());
	}
	return (outgoing_pipe_->output(cb));
}

void
PipeLink::pipe_splice_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	pipe_splice_action_->cancel();
	pipe_splice_action_ = NULL;

	switch (e.type_) {
	case Event::EOS:
	case Event::Error:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		return;
	}

	if (e.type_ == Event::Error)
		pipe_splice_error_ = true;

	delete pipe_splice_;
	pipe_splice_ = NULL;
}
