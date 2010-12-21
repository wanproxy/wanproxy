#ifndef	PIPE_SPLICE_H
#define	PIPE_SPLICE_H

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

#endif /* !PIPE_SPLICE_H */
