#ifndef	THREAD_H
#define	THREAD_H

struct ThreadState;

class Thread {
	friend struct ThreadState;

	std::string name_;
	ThreadState *state_;

protected:
	Thread(const std::string&);

public:
	virtual ~Thread();

	void join(void);
	void start(void);

	virtual void main(void) = 0;

public:
	static Thread *self(void);

	static bool stop_;
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
		NOTREACHED("/thread/null");
	}
};

#endif /* !THREAD_H */
