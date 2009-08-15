#ifndef	IO_PIPE_NULL_H
#define	IO_PIPE_NULL_H

class Action;
class EventCallback;

class PipeNull : public Pipe {
	Buffer input_buffer_;
	bool input_eos_;

	Action *output_action_;
	EventCallback *output_callback_;
public:
	PipeNull(void);
	~PipeNull();

	Action *input(Buffer *, EventCallback *);
	Action *output(EventCallback *);

private:
	void output_cancel(void);
};

#endif /* !IO_PIPE_NULL_H */
