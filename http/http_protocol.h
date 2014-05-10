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

#ifndef	HTTP_HTTP_PROTOCOL_H
#define	HTTP_HTTP_PROTOCOL_H

#include <map>

namespace HTTPProtocol {
	enum ParseStatus {
		ParseSuccess,
		ParseFailure,
		ParseIncomplete,
	};

	struct Message {
		enum Type {
			Request,
			Response,
		};

		Type type_;
		Buffer start_line_;
		std::map<std::string, std::vector<Buffer> > headers_;
		Buffer body_;
#if 0
		std::map<std::string, std::vector<Buffer> > trailers_;
#endif

		Message(Type type)
		: type_(type),
		  start_line_(),
		  headers_(),
		  body_()
		{ }

		~Message()
		{ }

		bool decode(Buffer *);
	};

	struct Request : public Message {
		Request(void)
		: Message(Message::Request)
		{ }

		~Request()
		{ }
	};

	struct Response : public Message {
		Response(void)
		: Message(Message::Response)
		{ }

		~Response()
		{ }
	};

	enum Status {
		OK,
		BadRequest,
		NotFound,
		NotImplemented,
		VersionNotSupported,
	};

	bool DecodeURI(Buffer *, Buffer *);
	ParseStatus ExtractLine(Buffer *, Buffer *, Buffer * = NULL);
}

#endif /* !HTTP_HTTP_PROTOCOL_H */
