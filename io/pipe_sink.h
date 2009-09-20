#ifndef	IO_PIPE_SINK_H
#define	IO_PIPE_SINK_H

class Action;
class EventCallback;

class PipeSink : public Pipe {
	bool input_eos_;

	Action *output_action_;
	EventCallback *output_callback_;
public:
	PipeSink(void);
	~PipeSink();

	Action *input(Buffer *, EventCallback *);
	Action *output(EventCallback *);

private:
	void output_cancel(void);
};

#endif /* !IO_PIPE_SINK_H */
