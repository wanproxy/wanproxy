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

#ifndef	EVENT_EVENT_POLL_H
#define	EVENT_EVENT_POLL_H

#include <map>

#include <common/thread/mutex.h>

struct EventPollState;

class EventPoll {
	friend class EventPollThread;
	friend class PollAction;

public:
	enum Type {
		Readable,
		Writable,
	};

private:
	class PollAction : public Cancellable {
		EventPoll *poll_;
		Type type_;
		int fd_;
	public:
		PollAction(EventPoll *poll, const Type& type, int fd)
		: poll_(poll),
		  type_(type),
		  fd_(fd)
		{ }

		~PollAction()
		{ }

		void cancel(void)
		{
			poll_->cancel(type_, fd_);
		}
	};

	struct PollHandler {
		EventCallback *callback_;
		Action *action_;

		PollHandler(void)
		: callback_(NULL),
		  action_(NULL)
		{ }

		~PollHandler()
		{
			ASSERT("/event/poll/handler", callback_ == NULL);
			ASSERT("/event/poll/handler", action_ == NULL);
		}

		void callback(Event);
		void cancel(void);
	};
	typedef std::map<int, PollHandler> poll_handler_map_t;

	LogHandle log_;
	Mutex mtx_;
	poll_handler_map_t read_poll_;
	poll_handler_map_t write_poll_;
	EventPollState *state_;

	EventPoll(void);
	~EventPoll();

	Action *poll(const Type&, int, EventCallback *);
	void cancel(const Type&, int);
	void wait(void);
	void signal(void);

	bool idle(void) const
	{
		return (read_poll_.empty() && write_poll_.empty());
	}
};

#endif /* !EVENT_EVENT_POLL_H */
