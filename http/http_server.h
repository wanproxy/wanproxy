/*
 * Copyright (c) 2011-2013 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	HTTP_HTTP_SERVER_H
#define	HTTP_HTTP_SERVER_H

#include <http/http_server_pipe.h>

#include <io/net/tcp_server.h>

#include <io/socket/simple_server.h>

class Splice;

class HTTPServerHandler {
protected:
	LogHandle log_;
private:
	Socket *client_;
protected:
	HTTPServerPipe *pipe_;
private:
	Splice *splice_;
	Action *splice_action_;
	Action *close_action_;
	Action *request_action_;
public:
	HTTPServerHandler(Socket *);
protected:
	virtual ~HTTPServerHandler();

private:
	void close_complete(void);
	void request(Event, HTTPProtocol::Request);
	void splice_complete(Event);

	virtual void handle_request(const std::string&, const std::string&, HTTPProtocol::Request) = 0;
};

template<typename T, typename Th, typename Ta = void>
class HTTPServer : public SimpleServer<T> {
	Ta arg_;
public:
	HTTPServer(Ta arg, SocketImpl impl, SocketAddressFamily family, const std::string& interface)
	: SimpleServer<T>("/http/server", impl, family, interface),
	  arg_(arg)
	{ }

	~HTTPServer()
	{ }

	void client_connected(Socket *client)
	{
		new Th(arg_, client);
	}
};

template<typename T, typename Th>
class HTTPServer<T, Th, void> : public SimpleServer<T> {
public:
	HTTPServer(SocketImpl impl, SocketAddressFamily family, const std::string& interface)
	: SimpleServer<T>("/http/server", impl, family, interface)
	{ }

	~HTTPServer()
	{ }

	void client_connected(Socket *client)
	{
		new Th(client);
	}
};

#endif /* !HTTP_HTTP_SERVER_H */
