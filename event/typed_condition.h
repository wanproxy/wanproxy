#ifndef	TYPED_CONDITION_H
#define	TYPED_CONDITION_H

#include <event/condition.h>

template<typename T>
class TypedCondition {
protected:
	TypedCondition(void)
	{ }

	~TypedCondition()
	{ }

public:
	virtual void signal(T) = 0;
	virtual Action *wait(TypedCallback<T> *) = 0;
};

template<typename T>
class TypedConditionVariable : public TypedCondition<T> {
	Action *wait_action_;
	TypedCallback<T> *wait_callback_;
public:
	TypedConditionVariable(void)
	: wait_action_(NULL),
	  wait_callback_(NULL)
	{ }

	~TypedConditionVariable()
	{
		ASSERT(wait_action_ == NULL);
		ASSERT(wait_callback_ == NULL);
	}

	void signal(T p)
	{
		if (wait_callback_ == NULL)
			return;
		ASSERT(wait_action_ == NULL);
		wait_callback_->param(p);
		wait_action_ = EventSystem::instance()->schedule(wait_callback_);
		wait_callback_ = NULL;
	}

	Action *wait(TypedCallback<T> *cb)
	{
		ASSERT(wait_action_ == NULL);
		ASSERT(wait_callback_ == NULL);

		wait_callback_ = cb;

		return (cancellation(this, &TypedConditionVariable::wait_cancel));
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

#endif /* !TYPED_CONDITION_H */
