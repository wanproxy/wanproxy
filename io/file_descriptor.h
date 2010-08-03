#ifndef	FILE_DESCRIPTOR_H
#define	FILE_DESCRIPTOR_H

#include <io/channel.h>

class FileDescriptor : public StreamChannel {
	LogHandle log_;
protected:
	int fd_;
public:
	FileDescriptor(int);
	~FileDescriptor();

	virtual Action *close(EventCallback *);
	virtual Action *read(size_t, EventCallback *);
	virtual Action *write(Buffer *, EventCallback *);
	virtual Action *shutdown(bool, bool, EventCallback *);
};

#endif /* !FILE_DESCRIPTOR_H */
