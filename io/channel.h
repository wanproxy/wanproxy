#ifndef	CHANNEL_H
#define	CHANNEL_H

class Action;
class EventCallback;

class Channel {
protected:
	Channel(void)
	{ }

public:
	virtual ~Channel()
	{ }

	virtual Action *close(EventCallback *) = 0;
	virtual Action *read(EventCallback *, size_t = 0) = 0;
	virtual Action *write(Buffer *, EventCallback *) = 0;
};

#endif /* !CHANNEL_H */
