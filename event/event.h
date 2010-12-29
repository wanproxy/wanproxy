#ifndef	EVENT_H
#define	EVENT_H

#include <common/buffer.h>

/*
 * The general-purpose event type.  Never extended or anything like that.
 * Tracking a user-specified pointer is handled by the callback, providers of
 * Events can put extra data into the data_ pointer, which is perhaps a bit
 * lacking.  (For example: the Socket::accept() codepath returns a pointer to
 * the client Socket there.)
 *
 * Because we are primarily a data-movement/processing system, a Buffer is an
 * integral part of every Event.  Plus, Buffers with no data are basically
 * free to copy, etc.
 *
 * Event handlers/callbacks always take a copy of the Event, which is subpar
 * but necessary since the first thing most of those callbacks do is to cancel
 * the Action that called them, which in turn deletes the underlying Callback
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
	void *data_;

	Event(void)
	: type_(Event::Invalid),
	  error_(0),
	  buffer_(),
	  data_(NULL)
	{ }

	Event(Type type)
	: type_(type),
	  error_(0),
	  buffer_(),
	  data_(NULL)
	{ }

	Event(Type type, int error)
	: type_(type),
	  error_(error),
	  buffer_(),
	  data_(NULL)
	{ }


	Event(Type type, const Buffer& buffer)
	: type_(type),
	  error_(0),
	  buffer_(buffer),
	  data_(NULL)
	{ }

	Event(Type type, void *data)
	: type_(type),
	  error_(0),
	  buffer_(),
	  data_(data)
	{ }

	Event(Type type, int error, const Buffer& buffer)
	: type_(type),
	  error_(error),
	  buffer_(buffer),
	  data_(NULL)
	{ }

	Event(Type type, int error, void *data)
	: type_(type),
	  error_(error),
	  buffer_(),
	  data_(data)
	{ }

	Event(Type type, int error, const Buffer& buffer, void *data)
	: type_(type),
	  error_(error),
	  buffer_(buffer),
	  data_(data)
	{ }

	Event(Type type, const Buffer& buffer, void *data)
	: type_(type),
	  error_(0),
	  buffer_(buffer),
	  data_(data)
	{ }

	Event(const Event& e)
	: type_(e.type_),
	  error_(e.error_),
	  buffer_(e.buffer_),
	  data_(e.data_)
	{ }

	Event& operator= (const Event& e)
	{
		type_ = e.type_;
		error_ = e.error_;
		buffer_ = e.buffer_;
		data_ = e.data_;
		return (*this);
	}

private:
	/* Prevent accidental void * use.  */
	Event(Type, const Buffer *);
	Event(Type, int, const Buffer *);
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

#endif /* !EVENT_H */
