#ifndef	ZLIB_INFLATE_PIPE_H
#define	ZLIB_INFLATE_PIPE_H

#include <io/pipe/pipe_producer.h>

#include <zlib.h>

class InflatePipe : public PipeProducer {
	z_stream stream_;
public:
	InflatePipe(void);
	~InflatePipe();

private:
	void consume(Buffer *);
};

#endif /* !ZLIB_INFLATE_PIPE_H */
