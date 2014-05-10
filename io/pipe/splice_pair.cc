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

#include <event/event_callback.h>

#include <io/pipe/splice.h>
#include <io/pipe/splice_pair.h>

/*
 * A SplicePair manages a pair of Splices.
 */

SplicePair::SplicePair(Splice *left, Splice *right)
: log_("/splice/pair"),
  left_(left),
  right_(right),
  callback_(NULL),
  callback_action_(NULL),
  left_action_(NULL),
  right_action_(NULL)
{
	ASSERT(log_, left_ != NULL);
	ASSERT(log_, right_ != NULL);
}

SplicePair::~SplicePair()
{
	ASSERT(log_, callback_ == NULL);
	ASSERT(log_, callback_action_ == NULL);
	ASSERT(log_, left_action_ == NULL);
	ASSERT(log_, right_action_ == NULL);
}

Action *
SplicePair::start(EventCallback *cb)
{
	ASSERT(log_, callback_ == NULL && callback_action_ == NULL);
	callback_ = cb;

	EventCallback *lcb = callback(this, &SplicePair::splice_complete, left_);
	left_action_ = left_->start(lcb);

	EventCallback *rcb = callback(this, &SplicePair::splice_complete, right_);
	right_action_ = right_->start(rcb);

	return (cancellation(this, &SplicePair::cancel));
}

void
SplicePair::cancel(void)
{
	if (callback_ != NULL) {
		delete callback_;
		callback_ = NULL;

		ASSERT(log_, callback_action_ == NULL);

		if (left_action_ != NULL) {
			left_action_->cancel();
			left_action_ = NULL;
		}

		if (right_action_ != NULL) {
			right_action_->cancel();
			right_action_ = NULL;
		}
	} else {
		ASSERT(log_, callback_action_ != NULL);
		callback_action_->cancel();
		callback_action_ = NULL;
	}
}

void
SplicePair::splice_complete(Event e, Splice *splice)
{
	ASSERT(log_, callback_ != NULL && callback_action_ == NULL);

	if (splice == left_) {
		left_action_->cancel();
		left_action_ = NULL;
	} else if (splice == right_) {
		right_action_->cancel();
		right_action_ = NULL;
	} else {
		NOTREACHED(log_);
	}

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
