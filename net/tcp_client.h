#ifndef	TCP_CLIENT_H
#define	TCP_CLIENT_H

struct TCPClient {
	static Action *connect(Socket **, const std::string&,
			       EventCallback *);
};

#endif /* !TCP_CLIENT_H */
