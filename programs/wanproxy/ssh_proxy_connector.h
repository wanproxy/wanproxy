/*
 * Copyright (c) 2008-2016 Juli Mallett. All rights reserved.
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

#ifndef	PROGRAMS_WANPROXY_SSH_PROXY_CONNECTOR_H
#define	PROGRAMS_WANPROXY_SSH_PROXY_CONNECTOR_H

#include <set>

#include <io/channel.h>

#include "ssh_stream.h"

class Pipe;
class PipePair;
struct SSHProxyConfig;
class Socket;
class Splice;
class SplicePair;
struct WANProxyCodec;

class SSHProxyConnector {
	friend class DestroyThread;

	LogHandle log_;

	Mutex mtx_;

	SimpleCallback::Method<SSHProxyConnector> stop_;
	Action *stop_action_;

	SimpleCallback::Method<SSHProxyConnector> local_close_complete_;
	Action *local_action_;
	Socket *local_socket_;

	SocketEventCallback::Method<SSHProxyConnector> connect_complete_;
	SimpleCallback::Method<SSHProxyConnector> remote_close_complete_;
	Action *remote_action_;
	Socket *remote_socket_;

	PipePair *pipe_pair_;

	SimpleCallback::Method<SSHProxyConnector> incoming_ssh_stream_complete_;
	SSHStream incoming_stream_;
	Action *incoming_stream_action_;
	Pipe *incoming_pipe_;
	Splice *incoming_splice_;

	SimpleCallback::Method<SSHProxyConnector> outgoing_ssh_stream_complete_;
	SSHStream outgoing_stream_;
	Action *outgoing_stream_action_;
	Pipe *outgoing_pipe_;
	Splice *outgoing_splice_;

	EventCallback::Method<SSHProxyConnector> splice_complete_;
	SplicePair *splice_pair_;
	Action *splice_action_;
public:
	SSHProxyConnector(const std::string&, PipePair *, Socket *, SocketImpl, SocketAddressFamily, const std::string&, const SSHProxyConfig *, WANProxyCodec *, WANProxyCodec *);
private:
	~SSHProxyConnector();

	void local_close_complete(void);
	void remote_close_complete(void);
	void connect_complete(Event, Socket *);
	void splice_complete(Event);
	void stop(void);

	void schedule_close(void);

	void incoming_ssh_stream_complete(void);
	void outgoing_ssh_stream_complete(void);
};

#endif /* !PROGRAMS_WANPROXY_SSH_PROXY_CONNECTOR_H */
