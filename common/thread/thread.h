#ifndef	THREAD_H
#define	THREAD_H

class Thread {
	std::string name_;

protected:
	Thread(const std::string& name)
	: name_(name)
	{ }

public:
	virtual ~Thread()
	{ }

	void join(void);
	void start(void);

	virtual void main(void) = 0;

public:
	static Thread *self(void);
};

#endif /* !THREAD_H */
