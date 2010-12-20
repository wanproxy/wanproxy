#ifndef	IO_PIPE_NULL_H
#define	IO_PIPE_NULL_H

#include <io/pipe/pipe_producer.h>

class PipeNull : public PipeProducer {
public:
	PipeNull(void)
	: PipeProducer("/io/pipe_null")
	{ }

	~PipeNull()
	{ }

private:
	void consume(Buffer *buf)
	{
		produce(buf);
	}
};

#endif /* !IO_PIPE_NULL_H */
