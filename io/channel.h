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
	virtual Action *read(size_t, EventCallback *) = 0;
	virtual Action *write(Buffer *, EventCallback *) = 0;

	virtual bool shutdown(bool, bool) = 0;
};

#endif /* !CHANNEL_H */
