#include <config/config_type_address_family.h>

struct ConfigTypeAddressFamily::Mapping config_type_address_family_map[] = {
	{ "IPv4",	ConfigAddressFamilyIPv4 },
	{ NULL,		ConfigAddressFamilyIPv4 }
};

ConfigTypeAddressFamily
	config_type_address_family("address-family", config_type_address_family_map);
