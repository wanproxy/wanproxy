#ifndef	IO_CHANNEL_H
#define	IO_CHANNEL_H

class Action;
class Buffer;

class Channel {
protected:
	Channel(void)
	{ }

public:
	virtual ~Channel()
	{ }

	/*
	 * NB:
	 * close does not take an EventCallback.  It can
	 * fail in any meaningful sense, but it absolutely
	 * must leave the Channel in a state in which it
	 * can be deleted, which is what callers care about,
	 * not whether it closed with an error.
	 */
	virtual Action *close(SimpleCallback *) = 0;

private:
	Action *close(EventCallback *);
};

class BlockChannel : public Channel {
protected:
	size_t bsize_;

	BlockChannel(size_t bsize)
	: bsize_(bsize)
	{ }

public:
	virtual ~BlockChannel()
	{ }

	virtual Action *read(off_t, EventCallback *) = 0;
	virtual Action *write(off_t, Buffer *, EventCallback *) = 0;
};

class StreamChannel : public Channel {
protected:
	StreamChannel(void)
	{ }

public:
	virtual ~StreamChannel()
	{ }

	virtual Action *read(size_t, EventCallback *) = 0;
	virtual Action *write(Buffer *, EventCallback *) = 0;

	virtual Action *shutdown(bool, bool, EventCallback *) = 0;
};

#endif /* !IO_CHANNEL_H */
