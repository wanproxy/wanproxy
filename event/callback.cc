#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event.h>
#include <event/event_system.h>

Action *
Callback::schedule(void)
{
	return (EventSystem::instance()->schedule(this));
}
