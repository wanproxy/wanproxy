#ifndef	IO_SOCKET_TYPES_H
#define	IO_SOCKET_TYPES_H

enum SocketAddressFamily {
	SocketAddressFamilyIP,
	SocketAddressFamilyIPv4,
	SocketAddressFamilyIPv6,
	SocketAddressFamilyUnix,
	SocketAddressFamilyUnspecified
};

enum SocketType {
	SocketTypeStream,
	SocketTypeDatagram,
};

#endif /* !IO_SOCKET_TYPES_H */
