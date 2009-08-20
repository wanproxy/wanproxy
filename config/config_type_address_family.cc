#include <config/config_type_address_family.h>

struct ConfigTypeAddressFamily::Mapping config_type_address_family_map[] = {
	{ "IP",		SocketAddressFamilyIP },
	{ "IPv4",	SocketAddressFamilyIPv4 },
	{ "IPv6",	SocketAddressFamilyIPv6 },
	{ "Unix",	SocketAddressFamilyUnix },
	{ NULL,		SocketAddressFamilyUnspecified }
};

ConfigTypeAddressFamily
	config_type_address_family("address-family", config_type_address_family_map);
