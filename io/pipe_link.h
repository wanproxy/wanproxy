#ifndef	PIPE_LINK_H
#define	PIPE_LINK_H

/*
 * There are many ways to do this; perhaps should put a Buffer in-between
 * and have input() call incoming_pipe_->input() then incoming_pipe_->output()
 * and don't call outcoming_pipe_->input() until output() is called?
 *
 * In theory we may need to rework this so that any spontaneous data from either
 * Pipe can be handled appropriately.
 */
class PipeLink : public Pipe {
	LogHandle log_;

	Pipe *incoming_pipe_;
	Pipe *outgoing_pipe_;

	Action *input_action_;
	EventCallback *input_callback_;
public:
	PipeLink(Pipe *, Pipe *);
	~PipeLink();

	Action *input(Buffer *, EventCallback *);
	Action *output(EventCallback *);
private:
	void input_cancel(void);
	void input_complete(Event);

	void incoming_input_complete(Event);
	void incoming_output_complete(Event);
	void outgoing_input_complete(Event);
};

#endif /* !PIPE_LINK_H */
