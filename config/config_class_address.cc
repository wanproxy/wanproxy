#include <config/config_class.h>
#include <config/config_class_address.h>

ConfigClassAddress config_class_address;

bool
ConfigClassAddress::Instance::activate(const ConfigObject *)
{
	switch (family_) {
	case SocketAddressFamilyIP:
	case SocketAddressFamilyIPv4:
	case SocketAddressFamilyIPv6:
		if (path_ != "") {
			ERROR("/config/class/address") << "IP socket has path field set, which is only valid for Unix domain sockets.";
			return (false);
		}
		return (true);

	case SocketAddressFamilyUnix:
		if (host_ != "" || port_ != "") {
			ERROR("/config/class/address") << "Unix domain socket has host and/or port field set, which is only valid for IP sockets.";
			return (false);
		}
		return (true);

	default:
		ERROR("/config/class/address") << "Unsupported address family.";
		return (false);
	}
}
