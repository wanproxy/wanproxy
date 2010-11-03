#ifndef	IO_PIPE_PAIR_SIMPLE_H
#define	IO_PIPE_PAIR_SIMPLE_H

#include <io/pipe_simple.h>
#include <io/pipe_simple_wrapper.h>

class PipePairSimple : public PipePair {
	PipeSimpleWrapper<PipePairSimple> *incoming_pipe_;
	PipeSimpleWrapper<PipePairSimple> *outgoing_pipe_;
protected:
	PipePairSimple(void)
	: incoming_pipe_(NULL),
	  outgoing_pipe_(NULL)
	{ }
public:
	virtual ~PipePairSimple()
	{
		if (incoming_pipe_ != NULL) {
			delete incoming_pipe_;
			incoming_pipe_ = NULL;
		}

		if (outgoing_pipe_ != NULL) {
			delete outgoing_pipe_;
			outgoing_pipe_ = NULL;
		}
	}

protected:
	virtual bool incoming_process(Buffer *, Buffer *) = 0;
	virtual bool outgoing_process(Buffer *, Buffer *) = 0;

public:
	Pipe *get_incoming(void)
	{
		ASSERT(incoming_pipe_ == NULL);
		incoming_pipe_ = new PipeSimpleWrapper<PipePairSimple>(this, &PipePairSimple::incoming_process);
		return (incoming_pipe_);
	}

	Pipe *get_outgoing(void)
	{
		ASSERT(outgoing_pipe_ == NULL);
		outgoing_pipe_ = new PipeSimpleWrapper<PipePairSimple>(this, &PipePairSimple::outgoing_process);
		return (outgoing_pipe_);
	}
};

#endif /* !IO_PIPE_PAIR_SIMPLE_H */
