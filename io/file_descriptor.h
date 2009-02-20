#ifndef	FILE_DESCRIPTOR_H
#define	FILE_DESCRIPTOR_H

#include <io/channel.h>

class FileDescriptor : public Channel {
	LogHandle log_;
protected:
	int fd_;
private:
	EventCallback *close_callback_;
	Action *close_action_;
	size_t read_amount_;
	Buffer read_buffer_;
	EventCallback *read_callback_;
	Action *read_action_;
	Buffer write_buffer_;
	EventCallback *write_callback_;
	Action *write_action_;
public:
	FileDescriptor(int);
	~FileDescriptor();

	Action *close(EventCallback *);
	Action *read(EventCallback *, size_t = 0);
	Action *write(Buffer *, EventCallback *);

private:
	void close_callback(void);
	void close_cancel(void);
	Action *close_schedule(void);

	void read_callback(Event, void *);
	void read_cancel(void);
	Action *read_schedule(void);

	void write_callback(Event, void *);
	void write_cancel(void);
	Action *write_schedule(void);
};

#endif /* !FILE_DESCRIPTOR_H */
