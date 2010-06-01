#ifndef	BLOCK_DEVICE_H
#define	BLOCK_DEVICE_H

#include <io/channel.h>

class BlockDevice : public BlockChannel {
	LogHandle log_;
protected:
	int fd_;
public:
	BlockDevice(int, size_t);
	~BlockDevice();

	virtual Action *close(EventCallback *);
	virtual Action *read(off_t, EventCallback *);
	virtual Action *write(off_t, Buffer *, EventCallback *);
};

#endif /* !BLOCK_DEVICE_H */
