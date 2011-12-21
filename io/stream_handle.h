#ifndef	IO_STREAM_HANDLE_H
#define	IO_STREAM_HANDLE_H

#include <io/channel.h>

class StreamHandle : public StreamChannel {
	LogHandle log_;
protected:
	int fd_;
public:
	StreamHandle(int);
	~StreamHandle();

	virtual Action *close(SimpleCallback *);
	virtual Action *read(size_t, EventCallback *);
	virtual Action *write(Buffer *, EventCallback *);
	virtual Action *shutdown(bool, bool, EventCallback *);
};

#endif /* !IO_STREAM_HANDLE_H */
