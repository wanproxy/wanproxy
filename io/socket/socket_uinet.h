/*
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

#ifndef	IO_SOCKET_SOCKET_UINET_H
#define	IO_SOCKET_SOCKET_UINET_H


#include <io/socket/socket.h>

struct uinet_socket;

class CallbackScheduler;

class SocketUinet : public Socket {

	struct uinet_socket *so_;
	LogHandle log_;
	CallbackScheduler *scheduler_;

	int last_upcall_state_;

	bool accept_do_;
	Action *accept_action_;
	SocketEventCallback *accept_callback_;

	bool connect_do_;
	Action *connect_action_;
	EventCallback *connect_callback_;

	bool read_do_;
	Action *read_action_;
	EventCallback *read_callback_;
	uint64_t read_amount_remaining_;
	Buffer read_buffer_;

	bool write_do_;
	Action *write_action_;
	EventCallback *write_callback_;
	uint64_t write_amount_remaining_;
	Buffer write_buffer_;

	SocketUinet(struct uinet_socket *, int, int, int);
public:
	~SocketUinet();

	virtual Action *close(SimpleCallback *);
	virtual Action *read(size_t, EventCallback *);
	virtual Action *write(Buffer *, EventCallback *);

	virtual Action *accept(SocketEventCallback *);
	virtual bool bind(const std::string&);
	virtual Action *connect(const std::string&, EventCallback *);
	virtual bool listen(void);
	virtual Action *shutdown(bool, bool, EventCallback *);

	virtual std::string getpeername(void) const;
	virtual std::string getsockname(void) const;

private:
	static int passive_receive_upcall(struct uinet_socket *, void *, int);
	static int active_receive_upcall(struct uinet_socket *, void *, int);
	static int active_send_upcall(struct uinet_socket *, void *, int);
	static int connect_upcall(struct uinet_socket *, void *, int);

	void accept_do(void);
	static void accept_upcall_prep(struct uinet_socket *, void *);
	void accept_cancel(void);

	void connect_cancel(void);

	void read_schedule(void);
	void read_callback(void);
	static void receive_upcall_prep(struct uinet_socket *, void *, int64_t);
	void read_cancel(void);

	void write_schedule(void);
	void write_callback(void);
	static void send_upcall_prep(struct uinet_socket *, void *, int64_t);
	void write_cancel(void);

public:
	static SocketUinet *create(SocketAddressFamily, SocketType, const std::string& = "", const std::string& = "");
};

#endif /* !IO_SOCKET_SOCKET_UINET_H */
