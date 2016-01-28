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

#include <io/pipe/splice.h>
#include <io/pipe/splice_pair.h>

/*
 * A SplicePair manages a pair of Splices.
 */

SplicePair::SplicePair(Splice *left, Splice *right)
: log_("/splice/pair"),
  mtx_("SplicePair"),
  left_(left),
  right_(right),
  cancel_(&mtx_, this, &SplicePair::cancel),
  callback_(NULL),
  callback_action_(NULL),
  left_splice_complete_(NULL, &mtx_, this, &SplicePair::left_splice_complete),
  left_action_(NULL),
  right_splice_complete_(NULL, &mtx_, this, &SplicePair::right_splice_complete),
  right_action_(NULL)
{
	ASSERT_NON_NULL(log_, left_);
	ASSERT_NON_NULL(log_, right_);
}

SplicePair::~SplicePair()
{
	ASSERT_NULL(log_, callback_);
	ASSERT_NULL(log_, callback_action_);
	ASSERT_NULL(log_, left_action_);
	ASSERT_NULL(log_, right_action_);
}

Action *
SplicePair::start(EventCallback *cb)
{
	ScopedLock _(&mtx_);
	ASSERT(log_, callback_ == NULL && callback_action_ == NULL);
	callback_ = cb;

	left_action_ = left_->start(&left_splice_complete_);
	right_action_ = right_->start(&right_splice_complete_);

	return (&cancel_);
}

void
SplicePair::cancel(void)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	if (callback_ != NULL) {
		callback_ = NULL;

		ASSERT_NULL(log_, callback_action_);

		if (left_action_ != NULL) {
			left_action_->cancel();
			left_action_ = NULL;
		}

		if (right_action_ != NULL) {
			right_action_->cancel();
			right_action_ = NULL;
		}
	} else {
		ASSERT_NON_NULL(log_, callback_action_);
		callback_action_->cancel();
		callback_action_ = NULL;
	}
}

void
SplicePair::left_splice_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	left_action_->cancel();
	left_action_ = NULL;

	splice_complete(e);
}

void
SplicePair::right_splice_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	right_action_->cancel();
	right_action_ = NULL;

	splice_complete(e);
}

void
SplicePair::splice_complete(Event e)
{
	ASSERT_LOCK_OWNED(log_, &mtx_);
	ASSERT(log_, callback_ != NULL && callback_action_ == NULL);

	switch (e.type_) {
	case Event::EOS:
		break;
	default:
		DEBUG(log_) << "Unexpected event: " << e;
		if (left_action_ != NULL) {
			left_action_->cancel();
			left_action_ = NULL;
		}
		if (right_action_ != NULL) {
			right_action_->cancel();
			right_action_ = NULL;
		}
		break;
	}

	if (left_action_ != NULL || right_action_ != NULL)
		return;

	ASSERT(log_, e.buffer_.empty());
	if (e.type_ == Event::EOS) {
		callback_->param(Event::Done);
	} else {
		ASSERT(log_, e.type_ != Event::Done);
		callback_->param(e);
	}
	callback_action_ = callback_->schedule();
	callback_ = NULL;
}
