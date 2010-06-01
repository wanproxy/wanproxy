#ifndef	BLOCK_CHANNEL_H
#define	BLOCK_CHANNEL_H

class Action;
class Buffer;

class BlockChannel {
protected:
	size_t bsize_;

	BlockChannel(size_t bsize)
	: bsize_(bsize)
	{ }

public:
	virtual ~BlockChannel()
	{ }

	virtual Action *close(EventCallback *) = 0;
	virtual Action *read(off_t, EventCallback *) = 0;
	virtual Action *write(off_t, Buffer *, EventCallback *) = 0;
};

#endif /* !BLOCK_CHANNEL_H */
