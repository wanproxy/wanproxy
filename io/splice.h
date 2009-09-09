#ifndef	SPLICE_H
#define	SPLICE_H

class Channel;
class Pipe;

class Splice {
	LogHandle log_;
	Channel *source_;
	Pipe *pipe_;
	Channel *sink_;

	EventCallback *callback_;
	Action *callback_action_;

	Action *read_action_;
	Action *input_action_;
	Action *output_action_;
	Action *write_action_;

public:
	Splice(Channel *, Pipe *, Channel *);
	~Splice();

	Action *start(EventCallback *);

private:
	void cancel(void);
	void complete(Event);

	void read_complete(Event);
	void input_complete(Event);

	void output_complete(Event);
	void write_complete(Event);
public:
	static Action *create(Splice **, Channel *, Pipe *, Channel *);
};

#endif /* !SPLICE_H */
