/*
 * Copyright (c) 2011 Juli Mallett. All rights reserved.
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

#ifndef	HTTP_HTTP_SERVER_PIPE_H
#define	HTTP_HTTP_SERVER_PIPE_H

#include <event/event.h>
#include <event/typed_pair_callback.h>

#include <http/http_protocol.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>

typedef class TypedPairCallback<Event, HTTPProtocol::Request> HTTPRequestEventCallback;

class HTTPServerPipe : public PipeProducer {
	enum State {
		GetStart,
		GetHeaders,

		GotRequest,
		Error
	};

	State state_;
	Buffer buffer_;

	HTTPProtocol::Request request_;
	std::string last_header_;

	Action *action_;
	HTTPRequestEventCallback *callback_;
public:
	HTTPServerPipe(const LogHandle&);
	~HTTPServerPipe();

	Action *request(HTTPRequestEventCallback *);

	/* XXX Use HTTPProtocol::Response type and add nice constructors for it?  */
	void send_response(HTTPProtocol::Status, Buffer *, Buffer *);
	void send_response(HTTPProtocol::Status, const std::string&, const std::string& = "text/plain");

private:
	void cancel(void);

	Action *schedule_callback(HTTPRequestEventCallback *);

	void consume(Buffer *);
};

#endif /* !HTTP_HTTP_SERVER_PIPE_H */
