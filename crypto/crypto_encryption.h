/*
 * Copyright (c) 2010-2013 Juli Mallett. All rights reserved.
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

#ifndef	CRYPTO_CRYPTO_ENCRYPTION_H
#define	CRYPTO_CRYPTO_ENCRYPTION_H

#include <set>

#include <event/event_callback.h>

namespace CryptoEncryption {
	class Method;

	enum Algorithm {
		TripleDES,
		AES128,
		AES192,
		AES256,
		Blowfish,
		CAST,
		IDEA,
		RC4,
	};

	enum Mode {
		CBC,
		CTR,
		Stream,
	};
	typedef	std::pair<Algorithm, Mode> Cipher;

	enum Operation {
		Encrypt,
		Decrypt,
	};

	class Session {
	protected:
		Session(void)
		{ }

	public:
		virtual ~Session()
		{ }

		virtual unsigned block_size(void) const = 0;
		virtual unsigned key_size(void) const = 0;
		virtual unsigned iv_size(void) const = 0;

		virtual Session *clone(void) const = 0;

		virtual bool initialize(Operation, const Buffer *, const Buffer *) = 0;

		virtual bool cipher(Buffer *, const Buffer *) = 0;

		virtual Action *submit(Buffer *, EventCallback *) = 0;
	};

	class Method {
		std::string name_;
	protected:
		Method(const std::string&);

		virtual ~Method()
		{ }
	public:
		virtual std::set<Cipher> ciphers(void) const = 0;
		virtual Session *session(Cipher) const = 0;

		static const Method *method(Cipher);
	};
}

std::ostream& operator<< (std::ostream&, CryptoEncryption::Algorithm);
std::ostream& operator<< (std::ostream&, CryptoEncryption::Mode);
std::ostream& operator<< (std::ostream&, CryptoEncryption::Cipher);

#endif /* !CRYPTO_CRYPTO_ENCRYPTION_H */
