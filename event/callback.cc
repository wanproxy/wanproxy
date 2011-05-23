#include <event/event_callback.h>
#include <event/event_system.h>

Action *
CallbackBase::schedule(void)
{
	if (scheduler_ != NULL)
		return (scheduler_->schedule(this));
	return (EventSystem::instance()->schedule(this));
}
