#ifndef	IO_PIPE_PIPE_PRODUCER_H
#define	IO_PIPE_PIPE_PRODUCER_H

class Action;

class PipeProducer : public Pipe {
protected:
	LogHandle log_;

private:
	Buffer output_buffer_;
	Action *output_action_;
	EventCallback *output_callback_;
	bool output_eos_;

	bool error_;
protected:
	PipeProducer(const LogHandle&);
	~PipeProducer();

	Action *input(Buffer *, EventCallback *);
	Action *output(EventCallback *);

private:
	void output_cancel(void);
	Action *output_do(EventCallback *);

public:
	void produce(Buffer *);
	void produce_eos(Buffer * = NULL);
	void produce_error(void);

protected:
	virtual void consume(Buffer *) = 0;
};

#endif /* !IO_PIPE_PIPE_PRODUCER_H */
