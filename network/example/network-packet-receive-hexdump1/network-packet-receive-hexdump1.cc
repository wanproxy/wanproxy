#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>
#include <event/timeout.h>

#include <network/network_interface.h>

static void usage(void);

int
main(int argc, char *argv[])
{
	const char *ifname;
	int ch;

	ifname = NULL;

	while ((ch = getopt(argc, argv, "i:")) != -1) {
		switch (ch) {
		case 'i':
			if (ifname != NULL)
				usage();
			ifname = optarg;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (ifname == NULL)
		usage();

	if (argc != 0)
		usage();

	NetworkInterface *interface = NetworkInterface::open(ifname);
	if (interface == NULL) {
		ERROR("/main") << "Unable to open interface.";
		return (1);
	}

	EventSystem::instance()->start();

	delete interface;
}

static void
usage(void)
{
	ERROR("/main") << "usage: network-packet-receive-hexdump1 -i ifname";
	exit(1);
}
