#ifndef	UDP_CLIENT_H
#define	UDP_CLIENT_H

struct UDPClient {
	static Action *connect(Socket **, const std::string&, int,
			       EventCallback *);
};

#endif /* !UDP_CLIENT_H */
