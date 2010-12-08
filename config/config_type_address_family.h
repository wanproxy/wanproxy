#ifndef	CONFIG_TYPE_ADDRESS_FAMILY_H
#define	CONFIG_TYPE_ADDRESS_FAMILY_H

#include <config/config_type_enum.h>

#include <io/socket/socket_types.h>

typedef ConfigTypeEnum<SocketAddressFamily> ConfigTypeAddressFamily;

extern ConfigTypeAddressFamily config_type_address_family;

#endif /* !CONFIG_TYPE_ADDRESS_FAMILY_H */
