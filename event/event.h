#ifndef	EVENT_H
#define	EVENT_H

/*
 * XXX Use Event::Attribute chains?
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

	Event(Type type, int error, void *data)
	: type_(type),
	  error_(error),
	  buffer_(),
	  data_(data)
	{ }

	Event(Type type, int error, const Buffer& buffer)
	: type_(type),
	  error_(error),
	  buffer_(buffer),
	  data_(NULL)
	{ }

	Event(Type type, int error)
	: type_(type),
	  error_(error),
	  buffer_(),
	  data_(NULL)
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
	return (os << e.type_ << '/' << e.error_ << " [" <<
		strerror(e.error_) << "]");
}

#endif /* !EVENT_H */
