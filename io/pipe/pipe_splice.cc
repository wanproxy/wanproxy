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

#include <common/thread/mutex.h>

#include <event/event_callback.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_splice.h>

/*
 * PipeSplice feeds the output from one Pipe to the input of another.
 */

PipeSplice::PipeSplice(Pipe *source, Pipe *sink)
: log_("/io/pipe/splice"),
  mtx_("PipeSplice"),
  source_(source),
  sink_(sink),
  source_eos_(false),
  output_complete_(NULL, &mtx_, this, &PipeSplice::output_complete),
  input_complete_(NULL, &mtx_, this, &PipeSplice::input_complete),
  cancel_(&mtx_, this, &PipeSplice::cancel),
  action_(NULL),
  callback_(NULL)
{ }

PipeSplice::~PipeSplice()
{
	ScopedLock _(&mtx_);
	ASSERT_NULL(log_, action_);
	ASSERT_NULL(log_, callback_);
}

Action *
PipeSplice::start(EventCallback *scb)
{
	ScopedLock _(&mtx_);
	ASSERT_NULL(log_, action_);
	ASSERT_NULL(log_, callback_);

	action_ = source_->output(&output_complete_);

	callback_ = scb;

	return (&cancel_);
}

void
PipeSplice::cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	if (action_ != NULL) {
		action_->cancel();
		action_ = NULL;
	}

	if (callback_ != NULL)
		callback_ = NULL;
}

void
PipeSplice::output_complete(Event e, Buffer buf)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	action_->cancel();
	action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
	case Event::Error:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		return;
	}

	ASSERT(log_, !source_eos_);

	if (e.type_ == Event::Error) {
		callback_->param(e);
		action_ = callback_->schedule();
		callback_ = NULL;

		return;
	}

	if (e.type_ == Event::EOS) {
		ASSERT(log_, buf.empty());
		source_eos_ = true;
	} else {
		ASSERT(log_, !buf.empty());
	}

	action_ = sink_->input(&buf, &input_complete_);
}

void
PipeSplice::input_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	action_->cancel();
	action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::Error:
		break;
	default:
		ERROR(log_) << "Unexpected event: " << e;
		return;
	}

	if (e.type_ == Event::Error) {
		callback_->param(e);
		action_ = callback_->schedule();
		callback_ = NULL;

		return;
	}

	if (source_eos_) {
		callback_->param(Event::EOS);
		action_ = callback_->schedule();
		callback_ = NULL;

		return;
	}

	action_ = source_->output(&output_complete_);
}
