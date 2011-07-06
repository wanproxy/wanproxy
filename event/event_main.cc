#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

void
event_main(void)
{
	EventSystem::instance()->start();

	EventSystem::instance()->join();
}
