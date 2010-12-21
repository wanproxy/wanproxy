#ifndef	IO_PIPE_SINK_H
#define	IO_PIPE_SINK_H

#include <io/pipe/pipe_producer.h>

class PipeSink : public PipeProducer {
public:
	PipeSink(void)
	: PipeProducer("/io/pipe_sink")
	{ }

	~PipeSink()
	{ }

private:
	void consume(Buffer *buf)
	{
		if (buf->empty())
			produce(buf);
		else
			buf->clear();
	}
};

#endif /* !IO_PIPE_SINK_H */
