#include <event/event_callback.h>
#include <event/event_poll.h>

void
EventPoll::PollHandler::callback(Event e)
{
	/*
	 * If EventPoll::poll() is called twice before the callback scheduled
	 * the first time can be run, bad things will happen.  Just return if
	 * we already have a pending callback.
	 */
	if (action_ != NULL && callback_ == NULL)
		return;
	ASSERT("/event/poll/handler", action_ == NULL);
	ASSERT("/event/poll/handler", callback_ != NULL);
	callback_->param(e);
	Action *a = callback_->schedule();
	callback_ = NULL;
	action_ = a;
}

void
EventPoll::PollHandler::cancel(void)
{
	if (callback_ != NULL) {
		delete callback_;
		callback_ = NULL;
		ASSERT("/event/poll/handler", action_ == NULL);
	} else {
		ASSERT("/event/poll/handler", action_ != NULL);
		action_->cancel();
		action_ = NULL;
	}
}
