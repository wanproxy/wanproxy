#ifndef	IO_PIPE_SIMPLE_H
#define	IO_PIPE_SIMPLE_H

class Action;
class EventCallback;

class PipeSimple : public Pipe {
	LogHandle log_;

	Buffer input_buffer_;
	bool input_eos_;

	Action *output_action_;
	EventCallback *output_callback_;
protected:
	PipeSimple(const LogHandle&);
	~PipeSimple();

	Action *input(Buffer *, EventCallback *);
	Action *output(EventCallback *);

private:
	void output_cancel(void);

protected:
	void output_spontaneous(void);

	virtual bool process(Buffer *, Buffer *) = 0;
};

#endif /* !IO_PIPE_SIMPLE_H */
