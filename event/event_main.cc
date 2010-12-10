#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

void
event_main(void)
{
	EventSystem::instance()->start();
}
