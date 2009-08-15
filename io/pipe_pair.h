#ifndef	IO_PIPE_PAIR_H
#define	IO_PIPE_PAIR_H

class Pipe;

class PipePair {
protected:
	PipePair(void)
	{ }
public:
	virtual ~PipePair()
	{ }

	virtual Pipe *get_incoming(void) = 0;
	virtual Pipe *get_outgoing(void) = 0;
};

#endif /* !IO_PIPE_PAIR_H */
