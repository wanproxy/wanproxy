#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>
#include <event/timeout.h>

#include <network/network_interface.h>

class PacketDumper {
	NetworkInterface *interface_;
	Action *receive_action_;
public:
	PacketDumper(NetworkInterface *interface)
	: interface_(interface),
	  receive_action_(NULL)
	{
		EventCallback *cb = callback(this, &PacketDumper::receive_complete);
		receive_action_ = interface_->receive(cb);
	}

	~PacketDumper()
	{
		if (receive_action_ != NULL) {
			receive_action_->cancel();
			receive_action_ = NULL;
		}
	}

private:
	void receive_complete(Event e)
	{
		receive_action_->cancel();
		receive_action_ = NULL;

		switch (e.type_) {
		case Event::Done:
			break;
		default:
			HALT("/packet/dumper") << "Unexpected event: " << e;
			return;
		}

		INFO("/packet/dumper") << e.buffer_.length() << " byte packet: " << std::endl << e.buffer_.hexdump();

		EventCallback *cb = callback(this, &PacketDumper::receive_complete);
		receive_action_ = interface_->receive(cb);
	}
};

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

	PacketDumper dumper(interface);

	EventSystem::instance()->start();

	delete interface;
}

static void
usage(void)
{
	ERROR("/main") << "usage: network-packet-receive-hexdump1 -i ifname";
	exit(1);
}