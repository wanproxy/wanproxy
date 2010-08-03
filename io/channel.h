#ifndef	CHANNEL_H
#define	CHANNEL_H

class Action;
class Buffer;

class Channel {
protected:
	Channel(void)
	{ }

public:
	virtual ~Channel()
	{ }

	virtual Action *close(EventCallback *) = 0;
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

#endif /* !CHANNEL_H */
