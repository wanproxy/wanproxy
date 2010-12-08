#ifndef	ZLIB_INFLATE_PIPE_H
#define	ZLIB_INFLATE_PIPE_H

#include <io/pipe/pipe_simple.h>

#include <zlib.h>

class InflatePipe : public PipeSimple {
	z_stream stream_;
public:
	InflatePipe(void);
	~InflatePipe();

private:
	bool process(Buffer *, Buffer *);
};

#endif /* !ZLIB_INFLATE_PIPE_H */
