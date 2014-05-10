/*
 * Copyright (c) 2012-2013 Juli Mallett. All rights reserved.
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

#ifndef	PROGRAMS_FWDPROXY_PROXY_CONNECTOR_H
#define	PROGRAMS_FWDPROXY_PROXY_CONNECTOR_H

#include <set>

class Pipe;
class PipePair;
class Socket;
class Splice;
class SplicePair;

class ProxyConnector {
	LogHandle log_;

	Action *stop_action_;

	Action *local_action_;
	Socket *local_socket_;

	Action *remote_action_;
	Socket *remote_socket_;

	PipePair *pipe_pair_;

	Pipe *incoming_pipe_;
	Splice *incoming_splice_;

	Pipe *outgoing_pipe_;
	Splice *outgoing_splice_;

	SplicePair *splice_pair_;
	Action *splice_action_;

public:
	ProxyConnector(const std::string&, PipePair *, Socket *, SocketAddressFamily, const std::string&);
private:
	~ProxyConnector();

	void close_complete(Socket *);
	void connect_complete(Event, Socket *);
	void splice_complete(Event);
	void stop(void);

	void schedule_close(void);
};

#endif /* !PROGRAMS_FWDPROXY_PROXY_CONNECTOR_H */
