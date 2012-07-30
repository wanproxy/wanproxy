#ifndef	CALLBACK_HANDLER_H
#define	CALLBACK_HANDLER_H

#include <event/action.h>
#include <event/callback.h>
#include <event/object_callback.h>

class CallbackHandler {
	Action *action_;
	SimpleCallback *callback_;
public:
	CallbackHandler(void)
	: action_(NULL),
	  callback_(NULL)
	{ }

	~CallbackHandler()
	{
		if (action_ != NULL) {
			action_->cancel();
			action_ = NULL;
		}

		ASSERT("/callback/handler", callback_ != NULL);
		delete callback_;
		callback_ = NULL;
	}

	SimpleCallback *callback(void)
	{
		ASSERT("/callback/handler", callback_ != NULL);
		return (::callback(this, &CallbackHandler::handle_callback));
	}

	void cancel(void)
	{
		ASSERT("/callback/handler", action_ != NULL);

		action_->cancel();
		action_ = NULL;
	}

	template<typename Tobj, typename Tmethod>
	void handler(Tobj obj, Tmethod method)
	{
		ASSERT("/callback/handler", action_ == NULL);
		ASSERT("/callback/handler", callback_ == NULL);

		callback_ = ::callback(obj, method);
	}

	template<typename Tobj, typename Tmethod, typename Targ>
	void handler(Tobj obj, Tmethod method, Targ arg)
	{
		ASSERT("/callback/handler", action_ == NULL);
		ASSERT("/callback/handler", callback_ == NULL);

		callback_ = ::callback(obj, method, arg);
	}

	void wait(Action *action)
	{
		ASSERT("/callback/handler", action_ == NULL);
		ASSERT("/callback/handler", action != NULL);

		action_ = action;
	}

private:
	void handle_callback(void)
	{
		ASSERT("/callback/handler", action_ != NULL);

		action_->cancel();
		action_ = NULL;

		callback_->execute();

		/*
		 * XXX
		 * This is a great place to hook in for debugging/tracing
		 * and to show whether another action was queued inside the
		 * callback execution path, etc.
		 */
	}
};

#endif /* !CALLBACK_HANDLER_H */
