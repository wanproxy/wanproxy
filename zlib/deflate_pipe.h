#ifndef	ZLIB_DEFLATE_PIPE_H
#define	ZLIB_DEFLATE_PIPE_H

#include <io/pipe/pipe_producer.h>

#include <zlib.h>

class DeflatePipe : public PipeProducer {
	z_stream stream_;
public:
	DeflatePipe(int = 0);
	~DeflatePipe();

private:
	void consume(Buffer *);
};

#endif /* !ZLIB_DEFLATE_PIPE_H */
