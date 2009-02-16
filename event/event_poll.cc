#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

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
	ASSERT(action_ == NULL);
	ASSERT(callback_ != NULL);
	callback_->event(e);
	Action *a = EventSystem::instance()->schedule(callback_);
	callback_ = NULL;
	action_ = a;
}

void
EventPoll::PollHandler::cancel(void)
{
	if (callback_ != NULL) {
		delete callback_;
		callback_ = NULL;
		ASSERT(action_ == NULL);
	} else {
		ASSERT(action_ != NULL);
		action_->cancel();
		action_ = NULL;
	}
}
