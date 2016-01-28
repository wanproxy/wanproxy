/*
 * Copyright (c) 2009-2013 Juli Mallett. All rights reserved.
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

#ifndef	PROGRAMS_WANPROXY_PROXY_SOCKS_CONNECTION_H
#define	PROGRAMS_WANPROXY_PROXY_SOCKS_CONNECTION_H

class ProxySocksConnection {
	friend class DestroyThread;

	enum State {
		GetSOCKSVersion,

		GetSOCKS4Command,
		GetSOCKS4Port,
		GetSOCKS4Address,
		GetSOCKS4User,

		GetSOCKS5AuthLength,
		GetSOCKS5Auth,
		GetSOCKS5Command,
		GetSOCKS5Reserved,
		GetSOCKS5AddressType,
		GetSOCKS5Address,
		GetSOCKS5Address6,
		GetSOCKS5NameLength,
		GetSOCKS5Name,
		GetSOCKS5Port,
	};

	LogHandle log_;
	Mutex mtx_;
	std::string name_;
	Socket *client_;
	SimpleCallback::Method<ProxySocksConnection> close_complete_;
	Action *action_;
	State state_;
	uint16_t network_port_;
	Buffer network_address_;
	bool socks5_authenticated_;
	std::string socks5_remote_name_;

public:
	ProxySocksConnection(const std::string&, Socket *);
private:
	~ProxySocksConnection();

private:
	void read_complete(Event);
	void write_complete(Event);
	void close_complete(void);

	void schedule_read(size_t);
	void schedule_write(void);
	void schedule_close(void);
};

#endif /* !PROGRAMS_WANPROXY_PROXY_SOCKS_CONNECTION_H */
