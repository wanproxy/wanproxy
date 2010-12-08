#ifndef	UNIX_CLIENT_H
#define	UNIX_CLIENT_H

struct UnixClient {
	static Action *connect(Socket **, const std::string&, EventCallback *);
};

#endif /* !UNIX_CLIENT_H */
