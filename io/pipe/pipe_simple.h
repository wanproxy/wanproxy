#ifndef	IO_PIPE_SIMPLE_H
#define	IO_PIPE_SIMPLE_H

#include <io/pipe/pipe_producer.h>

class PipeSimple : public PipeProducer {
protected:
	PipeSimple(const LogHandle&);
	~PipeSimple();

private:
	void consume(Buffer *);

protected:
	virtual bool process(Buffer *, Buffer *) = 0;
};

#endif /* !IO_PIPE_SIMPLE_H */
