#ifndef	IO_PIPE_PAIR_ECHO_H
#define	IO_PIPE_PAIR_ECHO_H

class PipePairEcho : public PipePair {
	class Half : public Pipe {
		Half *response_pipe_;

		bool source_eos_;

		Action *output_action_;
		Buffer output_buffer_;
		EventCallback *output_callback_;
	public:
		Half(Half *);
		~Half();

		Action *input(Buffer *, EventCallback *);
		Action *output(EventCallback *);

	private:
		void output_cancel(void);
	};

	Half incoming_pipe_;
	Half outgoing_pipe_;
public:
	PipePairEcho(void)
	: incoming_pipe_(&outgoing_pipe_),
	  outgoing_pipe_(&incoming_pipe_)
	{ }

	~PipePairEcho()
	{ }

	Pipe *get_incoming(void)
	{
		return (&incoming_pipe_);
	}

	Pipe *get_outgoing(void)
	{
		return (&outgoing_pipe_);
	}
};

#endif /* !IO_PIPE_PAIR_ECHO_H */
