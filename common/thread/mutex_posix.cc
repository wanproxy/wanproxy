/*
 * Copyright (c) 2010-2013 Juli Mallett. All rights reserved.
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

#include <errno.h>
#include <pthread.h>

#include <deque>

#include <common/thread/mutex.h>
#include <common/thread/thread.h>

#include "mutex_posix.h"

Mutex::Mutex(const std::string& name)
: Lock(name),
  state_(new MutexState())
{ }

Mutex::~Mutex()
{
	if (state_ != NULL) {
		delete state_;
		state_ = NULL;
	}
}

void
Mutex::assert_owned(bool owned, const LogHandle& log, const std::string& file, unsigned line, const std::string& function)
{
#ifdef NDEBUG
	(void)owned;
	(void)log;
	(void)file;
	(void)line;
	(void)function;
#else
	Thread::ID self = Thread::selfID();
	ASSERT_NON_NULL("/mutex/posix", self);

	state_->lock();
	if (state_->owner_ == NULL) {
		if (!owned) {
			state_->unlock();
			return;
		}
		HALT(log) << "Lock at " << file << ":" << line << " is not owned; in function " << function;
		state_->unlock();
		return;
	}
	if (state_->owner_ == self) {
		if (owned) {
			state_->unlock();
			return;
		}
		HALT(log) << "Lock at " << file << ":" << line << " is owned; in function " << function;
		state_->unlock();
		return;
	}
	if (!owned) {
		state_->unlock();
		return;
	}
	HALT(log) << "Lock at " << file << ":" << line << " is owned by another thread; in function " << function;
	state_->unlock();
#endif
}

void
Mutex::lock(void)
{
	state_->lock();
	state_->lock_acquire();
	state_->unlock();
}

bool
Mutex::try_lock(void)
{
	if (!state_->try_lock())
		return (false);
	bool success = state_->lock_acquire_try();
	state_->unlock();
	return (success);
}

void
Mutex::unlock(void)
{
	state_->lock();
	state_->lock_release();
	state_->unlock();
}
