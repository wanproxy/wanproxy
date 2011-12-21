#ifndef	IO_PIPE_PIPE_SPLICE_H
#define	IO_PIPE_PIPE_SPLICE_H

class PipeSplice {
	LogHandle log_;
	Pipe *source_;
	Pipe *sink_;
	bool source_eos_;
	Action *action_;
	EventCallback *callback_;
public:
	PipeSplice(Pipe *, Pipe *);
	~PipeSplice();

	Action *start(EventCallback *cb);
private:
	void cancel(void);

	void output_complete(Event);
	void input_complete(Event);
};

#endif /* !IO_PIPE_PIPE_SPLICE_H */
