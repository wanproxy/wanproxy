#ifndef	IO_BLOCK_HANDLE_H
#define	IO_BLOCK_HANDLE_H

#include <io/channel.h>

class BlockHandle : public BlockChannel {
	LogHandle log_;
protected:
	int fd_;
public:
	BlockHandle(int, size_t);
	~BlockHandle();

	virtual Action *close(SimpleCallback *);
	virtual Action *read(off_t, EventCallback *);
	virtual Action *write(off_t, Buffer *, EventCallback *);
};

#endif /* !IO_BLOCK_HANDLE_H */
