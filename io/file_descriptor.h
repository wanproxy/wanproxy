#ifndef	FILE_DESCRIPTOR_H
#define	FILE_DESCRIPTOR_H

#include <io/channel.h>

class FileDescriptor : public Channel {
	LogHandle log_;
protected:
	int fd_;
public:
	FileDescriptor(int);
	~FileDescriptor();

	Action *close(EventCallback *);
	Action *read(size_t, EventCallback *);
	Action *write(Buffer *, EventCallback *);
};

#endif /* !FILE_DESCRIPTOR_H */
