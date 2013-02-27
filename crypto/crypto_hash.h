/*
 * Copyright (c) 2010-2012 Juli Mallett. All rights reserved.
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

#ifndef	CRYPTO_CRYPTO_HASH_H
#define	CRYPTO_CRYPTO_HASH_H

#include <set>

#include <event/event_callback.h>

namespace CryptoHash {
	class Method;

	enum Algorithm {
		MD5,
		SHA1,
		SHA256,
		SHA512,
		RIPEMD160,
	};

	class Instance {
	protected:
		Instance(void)
		{ }

	public:
		virtual ~Instance()
		{ }

		virtual bool hash(Buffer *, const Buffer *) = 0;

		virtual Action *submit(Buffer *, EventCallback *) = 0;
	};

	class Method {
		std::string name_;
	protected:
		Method(const std::string&);

		virtual ~Method()
		{ }
	public:
		virtual std::set<Algorithm> algorithms(void) const = 0;
		virtual Instance *instance(Algorithm) const = 0;

		static const Method *method(Algorithm);
	};

	static inline Instance *instance(Algorithm algorithm)
	{
		const Method *method = Method::method(algorithm);
		if (method == NULL)
			return (NULL);
		return (method->instance(algorithm));
	}

	static inline bool hash(Algorithm algorithm, Buffer *out, const Buffer *in)
	{
		Instance *i = instance(algorithm);
		if (i == NULL)
			return (false);
		bool ok = i->hash(out, in);
		delete i;
		return (ok);
	}
}

std::ostream& operator<< (std::ostream&, CryptoHash::Algorithm);

#endif /* !CRYPTO_CRYPTO_HASH_H */
