#ifndef	PIPE_LINK_H
#define	PIPE_LINK_H

class PipeLink : public Pipe {
	LogHandle log_;

	Pipe *incoming_pipe_;
	Action *incoming_input_action_;
	bool incoming_output_eos_;
	Action *incoming_output_action_;

	Pipe *outgoing_pipe_;
	Action *outgoing_input_action_;
	Action *outgoing_output_action_;

	Action *input_action_;
	EventCallback *input_callback_;

	Buffer output_buffer_;
	bool output_eos_;
	Action *output_action_;
	EventCallback *output_callback_;
public:
	PipeLink(Pipe *, Pipe *);
	~PipeLink();

	Action *input(Buffer *, EventCallback *);
	Action *output(EventCallback *);
private:
	void input_cancel(void);
	void output_cancel(void);

	void incoming_input_complete(Event);
	void incoming_output_complete(Event);
	void outgoing_input_complete(Event);
	void outgoing_output_complete(Event);
};

#endif /* !PIPE_LINK_H */
