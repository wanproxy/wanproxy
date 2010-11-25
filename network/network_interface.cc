#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include <network/network_interface.h>
#include <network/network_interface_pcap.h>

NetworkInterface *
NetworkInterface::open(const std::string& ifname)
{
	NetworkInterface *interface = NULL;

#if defined(__FreeBSD__) || defined(__APPLE__)
	interface = NetworkInterfacePCAP::open(ifname);
	if (interface != NULL)
		return (interface);
#endif

	ASSERT(interface == NULL);
	ERROR("/network/interface") << "Unsupported network interface: " << ifname;
	return (NULL);
}
