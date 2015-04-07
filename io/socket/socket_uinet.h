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

#include <common/thread/mutex.h>

#include <io/socket/socket.h>

#include <uinet_api.h>

class CallbackScheduler;

#define	SOCKET_UINET_UPCALL_PASSIVE_RECEIVE	(0)
#define	SOCKET_UINET_UPCALL_ACTIVE_RECEIVE	(1)
#define	SOCKET_UINET_UPCALL_ACTIVE_SEND		(2)
#define	SOCKET_UINET_UPCALL_CONNECT		(3)
#define	SOCKET_UINET_UPCALLS			(4)

class SocketUinet : public Socket {
	LogHandle log_;
	CallbackScheduler *scheduler_;

	bool accept_do_;
	Action *accept_action_;
	SocketEventCallback *accept_callback_;
	Mutex accept_connect_mtx_;

	bool connect_do_;
	Action *connect_action_;
	EventCallback *connect_callback_;

	bool read_do_;
	Action *read_action_;
	EventCallback *read_callback_;
	uint64_t read_amount_remaining_;
	Buffer read_buffer_;
	Mutex read_mtx_;

	bool write_do_;
	Action *write_action_;
	EventCallback *write_callback_;
	uint64_t write_amount_remaining_;
	Buffer write_buffer_;
	Mutex write_mtx_;

	Mutex upcall_mtx_;
	Action *upcall_action_;
	bool upcall_pending_[SOCKET_UINET_UPCALLS];

protected:
	SocketUinet(struct uinet_socket *, int, int, int);

	struct uinet_socket *so_;

public:
	virtual ~SocketUinet();

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

protected:
	static int gettypenum(SocketType);
	static int getprotonum(const std::string&);
	static int getdomainnum(SocketAddressFamily, const std::string&);

	template <class S>
	static S *create_basic(SocketAddressFamily, SocketType, const std::string& = "", const std::string& = "");

private:
	void accept_schedule(void);
	void accept_callback(void);
	void accept_cancel(void);

	void connect_schedule(void);
	void connect_callback(void);
	void connect_cancel(void);

	void read_schedule(void);
	void read_callback(void);
	void read_cancel(void);

	void write_schedule(void);
	void write_callback(void);
	void write_cancel(void);

	void upcall_schedule(unsigned);
	void upcall_callback(void);
	bool upcall_do(void);

	static void accept_upcall_prep(struct uinet_socket *, void *);
	static void receive_upcall_prep(struct uinet_socket *, void *, int64_t, int64_t);
	static void send_upcall_prep(struct uinet_socket *, void *, int64_t);

	static int passive_receive_upcall(struct uinet_socket *, void *, int);
	static int active_receive_upcall(struct uinet_socket *, void *, int);
	static int active_send_upcall(struct uinet_socket *, void *, int);
	static int connect_upcall(struct uinet_socket *, void *, int);

public:
	static SocketUinet *create(SocketAddressFamily, SocketType, const std::string& = "", const std::string& = "");
};

template <class S>
S *
SocketUinet::create_basic(SocketAddressFamily family, SocketType type, const std::string& protocol, const std::string& hint)
{
	int typenum = gettypenum(type);
	if (-1 == typenum)
		return (NULL);

	int protonum = getprotonum(protocol);
	if (-1 == protonum)
		return (NULL);

	int domainnum = getdomainnum(family, hint);
	if (-1 == domainnum)
		return (NULL);

	struct uinet_socket *so;
	int error = uinet_socreate(domainnum, &so, typenum, protonum);
	if (error != 0) {
		/*
		 * If we were trying to create an IPv6 socket for a request that
		 * did not specify IPv4 vs. IPv6 and the system claims that the
		 * protocol is not supported, try explicitly creating an IPv4
		 * socket.
		 */
		if (uinet_inet6_enabled() && error == UINET_EPROTONOSUPPORT && domainnum == UINET_AF_INET6 &&
		    family == SocketAddressFamilyIP) {
			DEBUG("/socket/uinet") << "IPv6 socket create failed; trying IPv4.";
			return (S::create(SocketAddressFamilyIPv4, type, protocol, hint));
		}

		ERROR("/socket/uinet") << "Could not create socket: " << strerror(uinet_errno_to_os(error));
		return (NULL);
	}

	uinet_sosetnonblocking(so, 1);

	return (new S(so, domainnum, typenum, protonum));
}

#endif /* !IO_SOCKET_SOCKET_UINET_H */
