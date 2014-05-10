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

#ifndef	SSH_SSH_SERVER_HOST_KEY_H
#define	SSH_SSH_SERVER_HOST_KEY_H

class Buffer;

namespace SSH {
	struct Session;

	class ServerHostKey {
		std::string name_;
	protected:
		ServerHostKey(const std::string& xname)
		: name_(xname)
		{ }

	public:
		virtual ~ServerHostKey()
		{ }

		std::string name(void) const
		{
			return (name_);
		}

		virtual ServerHostKey *clone(void) const = 0;

		virtual bool decode_public_key(Buffer *) = 0;
		virtual void encode_public_key(Buffer *) const = 0;

		virtual bool sign(Buffer *, const Buffer *) const = 0;
		virtual bool verify(const Buffer *, const Buffer *) const = 0;

		static void add_client_algorithms(Session *);
		static ServerHostKey *server(Session *, const std::string&);
	};
}

#endif /* !SSH_SSH_SERVER_HOST_KEY_H */
