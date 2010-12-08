#ifndef	IO_PIPE_SIMPLE_H
#define	IO_PIPE_SIMPLE_H

class Action;

class PipeSimple : public Pipe {
protected:
	LogHandle log_;

private:
	Buffer input_buffer_;
	bool input_eos_;

	Buffer output_buffer_;
	Action *output_action_;
	EventCallback *output_callback_;

	bool process_error_;
protected:
	PipeSimple(const LogHandle&);
	~PipeSimple();

	Action *input(Buffer *, EventCallback *);
	Action *output(EventCallback *);

private:
	void output_cancel(void);
	Action *output_do(EventCallback *);

protected:
	virtual bool process(Buffer *, Buffer *) = 0;
};

#endif /* !IO_PIPE_SIMPLE_H */
