#ifndef	CONDITION_H
#define	CONDITION_H

class Condition {
protected:
	Condition(void)
	{ }

	virtual ~Condition()
	{ }

public:
	virtual void signal(void) = 0;
	virtual Action *wait(Callback *) = 0;
};

class ConditionVariable : public Condition {
	Action *wait_action_;
	Callback *wait_callback_;
public:
	ConditionVariable(void)
	: wait_action_(NULL),
	  wait_callback_(NULL)
	{ }

	~ConditionVariable()
	{
		ASSERT(wait_action_ == NULL);
		ASSERT(wait_callback_ == NULL);
	}

	void signal(void)
	{
		if (wait_callback_ == NULL)
			return;
		ASSERT(wait_action_ == NULL);
		wait_action_ = wait_callback_->schedule();
		wait_callback_ = NULL;
	}

	Action *wait(Callback *cb)
	{
		ASSERT(wait_action_ == NULL);
		ASSERT(wait_callback_ == NULL);

		wait_callback_ = cb;

		return (cancellation(this, &ConditionVariable::wait_cancel));
	}

private:
	void wait_cancel(void)
	{
		if (wait_callback_ != NULL) {
			ASSERT(wait_action_ == NULL);

			delete wait_callback_;
			wait_callback_ = NULL;
		} else {
			ASSERT(wait_action_ != NULL);

			wait_action_->cancel();
			wait_action_ = NULL;
		}
	}
};

#endif /* !CONDITION_H */
