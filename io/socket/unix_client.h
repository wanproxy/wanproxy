#ifndef	IO_SOCKET_UNIX_CLIENT_H
#define	IO_SOCKET_UNIX_CLIENT_H

struct UnixClient {
	static Action *connect(Socket **, const std::string&, EventCallback *);
};

#endif /* !IO_SOCKET_UNIX_CLIENT_H */
