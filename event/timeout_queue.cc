/*
 * Copyright (c) 2008-2013 Juli Mallett. All rights reserved.
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

#include <common/time/time.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/callback_queue.h>
#include <event/timeout_queue.h>

Action *
TimeoutQueue::append(uintmax_t ms, SimpleCallback *cb)
{
	NanoTime deadline = NanoTime::current_time();

	deadline.seconds_ += ms / 1000;
	deadline.nanoseconds_ += (ms % 1000) * 1000000;

	CallbackQueue *queue = timeout_queue_[deadline];
	if (queue == NULL) {
		queue = new CallbackQueue();
		timeout_queue_[deadline] = queue;
	}
	Action *a = queue->schedule(cb);
	return (a);
}

void
TimeoutQueue::perform(void)
{
	timeout_map_t::iterator it = timeout_queue_.begin();
	if (it == timeout_queue_.end())
		return;
	NanoTime now = NanoTime::current_time();
	if (it->first > now)
		return;
	CallbackQueue *queue = it->second;
	timeout_queue_.erase(it);
	queue->drain();
	delete queue;
}

bool
TimeoutQueue::ready(void) const
{
	timeout_map_t::const_iterator it = timeout_queue_.begin();
	if (it == timeout_queue_.end())
		return (false);
	NanoTime now = NanoTime::current_time();
	if (it->first <= now)
		return (true);
	return (false);
}
