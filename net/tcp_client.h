#ifndef	TCP_CLIENT_H
#define	TCP_CLIENT_H

struct TCPClient {
	static Action *connect(Socket **, const std::string&, int,
			       EventCallback *);
	static Action *connect(Socket **, uint32_t, uint16_t,
			       EventCallback *);
};

#endif /* !TCP_CLIENT_H */
