#include <event/event_callback.h>

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

	ASSERT("/network/interface", interface == NULL);
	ERROR("/network/interface") << "Unsupported network interface: " << ifname;
	return (NULL);
}
