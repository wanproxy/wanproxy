#ifndef	IO_PIPE_SPLICE_PAIR_H
#define	IO_PIPE_SPLICE_PAIR_H

class Splice;

class SplicePair {
	LogHandle log_;

	Splice *left_;
	Splice *right_;

	EventCallback *callback_;
	Action *callback_action_;

	Action *left_action_;
	Action *right_action_;
public:
	SplicePair(Splice *, Splice *);
	~SplicePair();

	Action *start(EventCallback *);
private:
	void cancel(void);

	void splice_complete(Event, Splice *);
};

#endif /* !IO_PIPE_SPLICE_PAIR_H */
