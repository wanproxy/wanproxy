/*
 * Copyright (c) 2008-2011 Juli Mallett. All rights reserved.
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
#include <event/event_poll.h>

void
EventPoll::PollHandler::callback(Event e)
{
	/*
	 * If EventPoll::poll() is called twice before the callback scheduled
	 * the first time can be run, bad things will happen.  Just return if
	 * we already have a pending callback.
	 */
	if (action_ != NULL && callback_ == NULL)
		return;
	ASSERT("/event/poll/handler", action_ == NULL);
	ASSERT("/event/poll/handler", callback_ != NULL);
	callback_->param(e);
	Action *a = callback_->schedule();
	callback_ = NULL;
	action_ = a;
}

void
EventPoll::PollHandler::cancel(void)
{
	if (callback_ != NULL) {
		delete callback_;
		callback_ = NULL;
		ASSERT("/event/poll/handler", action_ == NULL);
	} else {
		ASSERT("/event/poll/handler", action_ != NULL);
		action_->cancel();
		action_ = NULL;
	}
}
