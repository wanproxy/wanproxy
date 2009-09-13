#include <config/config_class.h>
#include <config/config_class_address.h>
#include <config/config_object.h>
#include <config/config_value.h>

ConfigClassAddress config_class_address;

bool
ConfigClassAddress::activate(ConfigObject *co)
{
	ConfigTypeAddressFamily *familyct;
	ConfigValue *familycv = co->get("family", &familyct);
	if (familycv == NULL) {
		ERROR("/config/class/address") << "Could not get address family.";
		return (false);
	}

	SocketAddressFamily family;
	if (!familyct->get(familycv, &family)) {
		ERROR("/config/class/address") << "Could not get address family.";
		return (false);
	}

	switch (family) {
	case SocketAddressFamilyIP:
	case SocketAddressFamilyIPv4:
	case SocketAddressFamilyIPv6:
		if (co->has("path")) {
			ERROR("/config/class/address") << "IP socket has path field set, which is only valid for Unix domain sockets.";
			return (false);
		}
		return (true);

	case SocketAddressFamilyUnix:
		if (co->has("host") || co->has("port")) {
			ERROR("/config/class/address") << "Unix domain socket has host and/or port field set, which is only valid for IP sockets.";
			return (false);
		}
		return (true);
	
	default:
		ERROR("/config/class/address") << "Unsupported address family.";
		return (false);
	}
}
