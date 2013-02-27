/*
 * Copyright (c) 2012 Juli Mallett. All rights reserved.
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

#ifndef	PROGRAMS_WANPROXY_SSH_STREAM_H
#define	PROGRAMS_WANPROXY_SSH_STREAM_H

#include <io/channel.h>

#include <ssh/ssh_session.h>

class Socket;
class Splice;
namespace SSH { class TransportPipe; }
struct WANProxyCodec;

class SSHStream : public StreamChannel {
	LogHandle log_;
	Socket *socket_;
	SSH::Session session_;
	WANProxyCodec *incoming_codec_;
	WANProxyCodec *outgoing_codec_;
	SSH::TransportPipe *pipe_;
	Splice *splice_;
	Action *splice_action_;
	SimpleCallback *start_callback_;
	Action *start_action_;
	EventCallback *read_callback_;
	Action *read_action_;
	Buffer input_buffer_;
	bool ready_;
	Action *ready_action_;
	EventCallback *write_callback_;
	Action *write_action_;
public:
	SSHStream(const LogHandle&, SSH::Role, WANProxyCodec *, WANProxyCodec *);
	~SSHStream();

	Action *start(Socket *socket, SimpleCallback *);

	Action *close(SimpleCallback *);
	Action *read(size_t, EventCallback *);
	Action *write(Buffer *, EventCallback *);
	Action *shutdown(bool, bool, EventCallback *);

private:
	void start_cancel(void);
	void read_cancel(void);

	void splice_complete(Event);

	void ready_complete(void);

	void receive_complete(Event);

	void write_cancel(void);
	void write_do(void);
};

#endif /* !PROGRAMS_WANPROXY_SSH_STREAM_H */
