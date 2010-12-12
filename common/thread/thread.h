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

class NullThread : public Thread {
public:
	NullThread(const std::string& name)
	: Thread(name)
	{ }

	~NullThread()
	{ }

private:
	void main(void)
	{
		NOTREACHED();
	}
};

#endif /* !THREAD_H */
