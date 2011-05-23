#ifndef	CALLBACK_H
#define	CALLBACK_H

#include <event/action.h>

class CallbackBase;

class CallbackScheduler {
protected:
	CallbackScheduler(void)
	{ }

public:
	virtual ~CallbackScheduler()
	{ }

	virtual Action *schedule(CallbackBase *) = 0;
};

class CallbackBase {
	CallbackScheduler *scheduler_;
protected:
	CallbackBase(CallbackScheduler *scheduler)
	: scheduler_(scheduler)
	{ }

public:
	virtual ~CallbackBase()
	{ }

public:
	virtual void execute(void) = 0;

	Action *schedule(void);
};

class SimpleCallback : public CallbackBase {
protected:
	SimpleCallback(CallbackScheduler *scheduler)
	: CallbackBase(scheduler)
	{ }

public:
	virtual ~SimpleCallback()
	{ }

protected:
	virtual void operator() (void) = 0;

	void execute(void)
	{
		(*this)();
	}
};

#include <event/object_callback.h>

#endif /* !CALLBACK_H */
