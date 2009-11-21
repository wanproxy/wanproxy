#ifndef	IO_PIPE_NULL_H
#define	IO_PIPE_NULL_H

#include <io/pipe_simple.h>

class PipeNull : public PipeSimple {
public:
	PipeNull(void);
	~PipeNull();

private:
	bool process(Buffer *, Buffer *);
};

#endif /* !IO_PIPE_NULL_H */
