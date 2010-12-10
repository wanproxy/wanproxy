#include <event/event_callback.h>
#include <event/event_system.h>

Action *
Callback::schedule(void)
{
	return (EventSystem::instance()->schedule(this));
}
