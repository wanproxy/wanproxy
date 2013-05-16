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

#ifndef	EVENT_TIMEOUT_QUEUE_H
#define	EVENT_TIMEOUT_QUEUE_H

#include <map>

#include <common/time/time.h>

class TimeoutQueue {
	typedef std::map<NanoTime, CallbackQueue> timeout_map_t;

	LogHandle log_;
	timeout_map_t timeout_queue_;
public:
	TimeoutQueue(void)
	: log_("/event/timeout/queue"),
	  timeout_queue_()
	{ }

	~TimeoutQueue()
	{ }

	bool empty(void) const
	{
		timeout_map_t::const_iterator it;

		/*
		 * Since we allow elements within each CallbackQueue to be
		 * cancelled, we must scan them.
		 *
		 * XXX
		 * We really shouldn't allow this, even if it means we have
		 * to specialize CallbackQueue for this purpose or add
		 * virtual methods to it.  As it is, we can return true
		 * for empty and for ready at the same time.  And in those
		 * cases we have to call perform to garbage collect the
		 * unused CallbackQueues.  We'll, quite conveniently,
		 * never make that call.  Yikes.
		 */
		for (it = timeout_queue_.begin(); it != timeout_queue_.end(); ++it) {
			if (it->second.empty())
				continue;
			return (false);
		}
		return (true);
	}

	NanoTime deadline(void) const
	{
		ASSERT(log_, !timeout_queue_.empty());
		return (timeout_queue_.begin()->first);
	}

	Action *append(uintmax_t, SimpleCallback *);
	void perform(void);
	bool ready(void) const;
};

#endif /* !EVENT_TIMEOUT_QUEUE_H */
