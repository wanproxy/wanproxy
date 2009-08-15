#ifndef	IO_PIPE_H
#define	IO_PIPE_H

class Action;
class Buffer;
class EventCallback;

class Pipe {
protected:
	Pipe(void)
	{ }
public:
	virtual ~Pipe()
	{ }

	virtual Action *input(Buffer *, EventCallback *) = 0;
	virtual Action *output(EventCallback *) = 0;
};

#endif /* !IO_PIPE_H */
