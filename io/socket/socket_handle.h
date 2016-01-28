/*
 * Copyright (c) 2008-2016 Juli Mallett. All rights reserved.
 * Copyright (c) 2013 Patrick Kelsey. All rights reserved.
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

#ifndef	IO_SOCKET_SOCKET_HANDLE_H
#define	IO_SOCKET_SOCKET_HANDLE_H

#include <event/cancellation.h>

#include <io/stream_handle.h>
#include <io/socket/socket.h>

class SocketHandle : public Socket, public StreamHandle {
	using StreamHandle::close;

	LogHandle log_;
	Mutex mtx_;

	EventCallback::Method<SocketHandle> accept_poll_complete_;
	Cancellation<SocketHandle> accept_cancel_;
	Action *accept_action_;
	SocketEventCallback *accept_callback_;

	EventCallback::Method<SocketHandle> connect_poll_complete_;
	Cancellation<SocketHandle> connect_cancel_;
	EventCallback *connect_callback_;
	Action *connect_action_;

	SocketHandle(int, int, int, int);
public:
	~SocketHandle();

	virtual Action *accept(SocketEventCallback *);
	virtual bool bind(const std::string&);
	virtual Action *connect(const std::string&, EventCallback *);
	virtual bool listen(void);
	virtual Action *shutdown(bool, bool, EventCallback *);

	virtual std::string getpeername(void) const;
	virtual std::string getsockname(void) const;

private:
	void accept_poll_complete(Event);
	void accept_cancel(void);
	Action *accept_schedule(void);

	void connect_poll_complete(Event);
	void connect_cancel(void);
	Action *connect_schedule(void);

public:
	static SocketHandle *create(SocketAddressFamily, SocketType, const std::string& = "", const std::string& = "");
};

#endif /* !IO_SOCKET_SOCKET_HANDLE_H */
