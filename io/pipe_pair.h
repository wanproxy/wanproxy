#ifndef	IO_PIPE_PAIR_H
#define	IO_PIPE_PAIR_H

class Pipe;

class PipePair {
protected:
	PipePair(void)
	{ }
public:
	/*
	 * NB
	 * It is assumed that a PipePair will handle freeing any memory
	 * associated with the Pipes returned by get_incoming() and
	 * get_outgoing().
	 */
	virtual ~PipePair()
	{ }

	virtual Pipe *get_incoming(void) = 0;
	virtual Pipe *get_outgoing(void) = 0;
};

#endif /* !IO_PIPE_PAIR_H */
