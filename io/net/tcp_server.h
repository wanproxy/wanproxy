/*
 * Copyright (c) 2008-2011 Juli Mallett. All rights reserved.
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

#ifndef	IO_NET_TCP_SERVER_H
#define	IO_NET_TCP_SERVER_H

#include <io/socket/socket.h>

class TCPServer {
	LogHandle log_;
	Socket *socket_;

	TCPServer(Socket *socket)
	: log_("/tcp/server"),
	  socket_(socket)
	{ }

public:
	~TCPServer()
	{
		if (socket_ != NULL) {
			delete socket_;
			socket_ = NULL;
		}
	}

	Action *accept(SocketEventCallback *cb)
	{
		return (socket_->accept(cb));
	}

	Action *close(SimpleCallback *cb)
	{
		return (socket_->close(cb));
	}

	std::string getsockname(void) const
	{
		return (socket_->getsockname());
	}

	static TCPServer *listen(SocketAddressFamily, const std::string&);
};

#endif /* !IO_NET_TCP_SERVER_H */
