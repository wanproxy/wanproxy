#ifndef	PIPE_LINK_H
#define	PIPE_LINK_H

class PipeSplice;

class PipeLink : public Pipe {
	LogHandle log_;

	Pipe *incoming_pipe_;
	Pipe *outgoing_pipe_;
	PipeSplice *pipe_splice_;
	Action *pipe_splice_action_;
	bool pipe_splice_error_;
public:
	PipeLink(Pipe *, Pipe *);
	~PipeLink();

	Action *input(Buffer *, EventCallback *);
	Action *output(EventCallback *);
private:
	void pipe_splice_complete(Event);
};

#endif /* !PIPE_LINK_H */
