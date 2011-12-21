#ifndef	EVENT_CONDITION_H
#define	EVENT_CONDITION_H

class Condition {
protected:
	Condition(void)
	{ }

	virtual ~Condition()
	{ }

public:
	virtual void signal(void) = 0;
	virtual Action *wait(SimpleCallback *) = 0;
};

class ConditionVariable : public Condition {
	Action *wait_action_;
	SimpleCallback *wait_callback_;
public:
	ConditionVariable(void)
	: wait_action_(NULL),
	  wait_callback_(NULL)
	{ }

	~ConditionVariable()
	{
		ASSERT("/condition/variable", wait_action_ == NULL);
		ASSERT("/condition/variable", wait_callback_ == NULL);
	}

	void signal(void)
	{
		if (wait_callback_ == NULL)
			return;
		ASSERT("/condition/variable", wait_action_ == NULL);
		wait_action_ = wait_callback_->schedule();
		wait_callback_ = NULL;
	}

	Action *wait(SimpleCallback *cb)
	{
		ASSERT("/condition/variable", wait_action_ == NULL);
		ASSERT("/condition/variable", wait_callback_ == NULL);

		wait_callback_ = cb;

		return (cancellation(this, &ConditionVariable::wait_cancel));
	}

private:
	void wait_cancel(void)
	{
		if (wait_callback_ != NULL) {
			ASSERT("/condition/variable", wait_action_ == NULL);

			delete wait_callback_;
			wait_callback_ = NULL;
		} else {
			ASSERT("/condition/variable", wait_action_ != NULL);

			wait_action_->cancel();
			wait_action_ = NULL;
		}
	}
};

#endif /* !EVENT_CONDITION_H */
