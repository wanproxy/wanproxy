#ifndef	ZLIB_DEFLATE_PIPE_H
#define	ZLIB_DEFLATE_PIPE_H

#include <io/pipe_simple.h>

#include <zlib.h>

class DeflatePipe : public PipeSimple {
	z_stream stream_;
	bool finished_;
public:
	DeflatePipe(int = 0);
	~DeflatePipe();

private:
	bool process(Buffer *, Buffer *);
};

#endif /* !ZLIB_DEFLATE_PIPE_H */
