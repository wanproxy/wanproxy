#ifndef	IO_PIPE_PIPE_H
#define	IO_PIPE_PIPE_H

class Action;
class Buffer;

/*
 * XXX
 * Should Pipe::output take a length?
 */
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

#endif /* !IO_PIPE_PIPE_H */
