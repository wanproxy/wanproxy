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

#include <event/event_callback.h>
#include <event/event_system.h>

CallbackBase::CallbackBase(CallbackScheduler *scheduler, Lock *xlock)
: scheduler_(scheduler),
  lock_(xlock),
  scheduled_(false),
  next_(NULL),
  prev_(NULL)
{
	/*
	 * Use the default scheduler if we haven't been given one.
	 *
	 * This uses the legacy callback thread, as it were, which
	 * does not distribute callbacks over all available workers,
	 * but rather runs naive callbacks all in the same thread.
	 *
	 * Eventually this needs to pick a worker using the lock
	 * as an index into the available pool.
	 */
	if (scheduler_ == NULL)
		scheduler_ = EventSystem::instance()->scheduler();
}

void
CallbackBase::cancel(void)
{
	ASSERT_LOCK_OWNED("/callback/base", lock_);
	ASSERT_NON_NULL("/callback/base", scheduler_);
	if (!scheduled_)
		return;
	scheduler_->cancel(this);
	scheduled_ = false;
}

void
CallbackBase::deschedule(void)
{
	ASSERT_LOCK_OWNED("/callback/base", lock_);
	ASSERT("/callback/base", scheduled_);
	scheduled_ = false;
	execute();
	lock_->unlock();
}
