#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

void
event_main(void)
{
	/* XXX Block SIGINT in the initial thread using pthread_sigmask?  */
	EventSystem::instance()->start();

	EventSystem::instance()->join();
}
