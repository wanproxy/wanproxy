#ifndef	HTTP_HTTP_SERVER_H
#define	HTTP_HTTP_SERVER_H

#include <http/http_server_pipe.h>

#include <io/net/tcp_server.h>

#include <io/socket/simple_server.h>

class Splice;

class HTTPServerHandler {
	LogHandle log_;
	Socket *client_;
protected:
	HTTPServerPipe *pipe_;
private:
	Splice *splice_;
	Action *splice_action_;
	Action *close_action_;
	Action *message_action_;
public:
	HTTPServerHandler(Socket *);
protected:
	virtual ~HTTPServerHandler();

private:
	void close_complete(void);
	void request(Event, HTTPProtocol::Message);
	void splice_complete(Event);

	virtual void handle_request(const std::string&, const std::string&, HTTPProtocol::Message) = 0;
};

template<typename T, typename Th>
class HTTPServer : public SimpleServer<T> {
public:
	HTTPServer(SocketAddressFamily family, const std::string& interface)
	: SimpleServer<T>("/http/server", family, interface)
	{ }

	~HTTPServer()
	{ }

	void client_connected(Socket *client)
	{
		new Th(client);
	}
};

#endif /* !HTTP_HTTP_SERVER_H */
