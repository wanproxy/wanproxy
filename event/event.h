/*
 * Copyright (c) 2008-2012 Juli Mallett. All rights reserved.
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

#ifndef	EVENT_EVENT_H
#define	EVENT_EVENT_H

#include <common/buffer.h>

/*
 * The general-purpose event type.  Never extended or anything like that.
 * Tracking a user-specified pointer is handled by the callback, providers of
 * Events can pass extra data by extending their Callback type, e.g. the
 * SocketEventCallback used by Socket::accept() to pass back a Socket pointer
 * along with an Event.
 * XXX In light of this extension, it may make sense to move the Buffer out
 *     of Event now, since the Callback can handle Buffers independently.
 *
 * Because we are primarily a data-movement/processing system, a Buffer is an
 * integral part of every Event.  Plus, Buffers with no data are basically
 * free to copy, etc.
 *
 * Event handlers/callbacks always take a copy of the Event, which is subpar
 * but necessary since the first thing most of those callbacks do is to cancel
 * the Action that called them, which in turn deletes the underlying SimpleCallback
 * object, which would in turn delete the holder of the associated Event if a
 * reference or pointer were passed.  One can argue that the right thing to do
 * is process the event fully before cancelling the Action, but that is not
 * how things are done at present.
 */
struct Event {
	enum Type {
		Invalid,
		Done,
		EOS,
		Error,
	};

	Type type_;
	int error_;
	Buffer buffer_;

	Event(void)
	: type_(Event::Invalid),
	  error_(0),
	  buffer_()
	{ }

	Event(Type type)
	: type_(type),
	  error_(0),
	  buffer_()
	{ }

	Event(Type type, int error)
	: type_(type),
	  error_(error),
	  buffer_()
	{ }


	Event(Type type, const Buffer& buffer)
	: type_(type),
	  error_(0),
	  buffer_(buffer)
	{ }

	Event(Type type, int error, const Buffer& buffer)
	: type_(type),
	  error_(error),
	  buffer_(buffer)
	{ }

	Event(const Event& e)
	: type_(e.type_),
	  error_(e.error_),
	  buffer_(e.buffer_)
	{ }

	Event& operator= (const Event& e)
	{
		type_ = e.type_;
		error_ = e.error_;
		buffer_ = e.buffer_;
		return (*this);
	}
};

static inline std::ostream&
operator<< (std::ostream& os, Event::Type type)
{
	switch (type) {
	case Event::Invalid:
		return (os << "<Invalid>");
	case Event::Done:
		return (os << "<Done>");
	case Event::EOS:
		return (os << "<EOS>");
	case Event::Error:
		return (os << "<Error>");
	default:
		return (os << "<Unexpected Event::Type>");
	}
}

static inline std::ostream&
operator<< (std::ostream& os, Event e)
{
	if (e.type_ != Event::Error && e.error_ == 0)
		return (os << e.type_);
	return (os << e.type_ << '/' << e.error_ << " [" <<
		strerror(e.error_) << "]");
}

#endif /* !EVENT_EVENT_H */
