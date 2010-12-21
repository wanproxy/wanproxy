#ifndef	IO_PIPE_PAIR_ECHO_H
#define	IO_PIPE_PAIR_ECHO_H

#include <io/pipe/pipe_pair_producer.h>

class PipePairEcho : public PipePairProducer {
public:
	PipePairEcho(void)
	: PipePairProducer("/pipe/pair/echo")
	{ }

	~PipePairEcho()
	{ }

	void incoming_consume(Buffer *buf)
	{
		outgoing_produce(buf);
	}

	void outgoing_consume(Buffer *buf)
	{
		incoming_produce(buf);
	}
};

#endif /* !IO_PIPE_PAIR_ECHO_H */
