/*
 * Copyright (c) 2009-2016 Juli Mallett. All rights reserved.
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

#include <io/channel.h>
#include <io/pipe/pipe.h>
#include <io/pipe/splice.h>

/*
 * A Splice passes data unidirectionally between StreamChannels across a Pipe.
 */

Splice::Splice(const LogHandle& log, StreamChannel *source, Pipe *pipe, StreamChannel *sink)
: log_(""),
  mtx_("Splice"),
  source_(source),
  pipe_(pipe),
  sink_(sink),
  cancel_(&mtx_, this, &Splice::cancel),
  callback_(NULL),
  callback_action_(NULL),
  read_complete_(NULL, &mtx_, this, &Splice::read_complete),
  read_eos_(false),
  read_action_(NULL),
  input_complete_(NULL, &mtx_, this, &Splice::input_complete),
  input_action_(NULL),
  output_complete_(NULL, &mtx_, this, &Splice::output_complete),
  output_eos_(false),
  output_action_(NULL),
  write_complete_(NULL, &mtx_, this, &Splice::write_complete),
  write_action_(NULL),
  shutdown_complete_(NULL, &mtx_, this, &Splice::shutdown_complete),
  shutdown_action_(NULL)
{
	log_ = log + "/splice";

	ASSERT_NON_NULL(log_, source_);
	ASSERT_NON_NULL(log_, sink_);
}

Splice::~Splice()
{
	ScopedLock _(&mtx_);
	ASSERT_NULL(log_, callback_);
	ASSERT_NULL(log_, callback_action_);
	ASSERT_NULL(log_, read_action_);
	ASSERT_NULL(log_, input_action_);
	ASSERT_NULL(log_, output_action_);
	ASSERT_NULL(log_, write_action_);
	ASSERT_NULL(log_, shutdown_action_);
}

Action *
Splice::start(EventCallback *cb)
{
	ScopedLock _(&mtx_);
	ASSERT(log_, callback_ == NULL && callback_action_ == NULL);
	callback_ = cb;

	read_action_ = source_->read(0, &read_complete_);

	if (pipe_ != NULL)
		output_action_ = pipe_->output(&output_complete_);

	return (&cancel_);
}

void
Splice::cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	if (callback_ != NULL) {
		callback_ = NULL;

		ASSERT_NULL(log_, callback_action_);

		if (read_action_ != NULL) {
			read_action_->cancel();
			read_action_ = NULL;
		}

		if (input_action_ != NULL) {
			input_action_->cancel();
			input_action_ = NULL;
		}

		if (output_action_ != NULL) {
			output_action_->cancel();
			output_action_ = NULL;
		}

		if (write_action_ != NULL) {
			write_action_->cancel();
			write_action_ = NULL;
		}

		if (shutdown_action_ != NULL) {
			shutdown_action_->cancel();
			shutdown_action_ = NULL;
		}
	} else {
		ASSERT_NON_NULL(log_, callback_action_);
		callback_action_->cancel();
		callback_action_ = NULL;
	}
}

void
Splice::complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	ASSERT_NON_NULL(log_, callback_);
	ASSERT_NULL(log_, callback_action_);

	if (read_action_ != NULL) {
		read_action_->cancel();
		read_action_ = NULL;
	}

	if (input_action_ != NULL) {
		input_action_->cancel();
		input_action_ = NULL;
	}

	if (output_action_ != NULL) {
		output_action_->cancel();
		output_action_ = NULL;
	}

	if (write_action_ != NULL) {
		write_action_->cancel();
		write_action_ = NULL;
	}

	if (shutdown_action_ != NULL) {
		shutdown_action_->cancel();
		shutdown_action_ = NULL;
	}

	callback_->param(e);
	callback_action_ = callback_->schedule();
	callback_ = NULL;
}

void
Splice::read_complete(Event e, Buffer buf)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	read_action_->cancel();
	read_action_ = NULL;

	ASSERT(log_, !read_eos_);

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		complete(e);
		return;
	}

	if (buf.empty()) {
		ASSERT(log_, e.type_ == Event::EOS);

		read_eos_ = true;
	}

	if (pipe_ != NULL) {
		ASSERT_NULL(log_, input_action_);
		input_action_ = pipe_->input(&buf, &input_complete_);
	} else {
		if (e.type_ == Event::EOS && buf.empty()) {
			shutdown_action_ = sink_->shutdown(false, true, &shutdown_complete_);
			return;
		}

		ASSERT_NULL(log_, write_action_);
		write_action_ = sink_->write(&buf, &write_complete_);
	}
}

void
Splice::input_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	input_action_->cancel();
	input_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		complete(e);
		return;
	}

	ASSERT_NULL(log_, read_action_);
	if (!read_eos_)
		read_action_ = source_->read(0, &read_complete_);
	else if (output_eos_)
		complete(Event::EOS);
}

void
Splice::output_complete(Event e, Buffer buf)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	output_action_->cancel();
	output_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::EOS:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		complete(e);
		return;
	}

	if (e.type_ == Event::EOS && buf.empty()) {
		shutdown_action_ = sink_->shutdown(false, true, &shutdown_complete_);
		return;
	}

	ASSERT_NULL(log_, write_action_);
	write_action_ = sink_->write(&buf, &write_complete_);
}

void
Splice::write_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	write_action_->cancel();
	write_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		complete(e);
		return;
	}

	if (pipe_ != NULL) {
		ASSERT_NULL(log_, output_action_);
		output_action_ = pipe_->output(&output_complete_);
	} else {
		if (read_eos_) {
			if (shutdown_action_ == NULL) {
				shutdown_action_ = sink_->shutdown(false, true, &shutdown_complete_);
				return;
			}
		} else {
			read_action_ = source_->read(0, &read_complete_);
		}
	}
}

void
Splice::shutdown_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	shutdown_action_->cancel();
	shutdown_action_ = NULL;

	switch (e.type_) {
	case Event::Done:
	case Event::Error:
		break;
	default:
		HALT(log_) << "Unexpected event: " << e;
		return;
	}

	if (e.type_ == Event::Error)
		DEBUG(log_) << "Could not shut down write channel.";
	if (read_eos_) {
		complete(Event::EOS);
	} else {
		output_eos_ = true;
	}
}
