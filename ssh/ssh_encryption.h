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

#ifndef	SSH_SSH_ENCRYPTION_H
#define	SSH_SSH_ENCRYPTION_H

#include <crypto/crypto_encryption.h>

namespace SSH {
	struct Session;

	class Encryption {
	protected:
		const std::string name_;
		const unsigned block_size_;
		const unsigned key_size_;
		const unsigned iv_size_;

		Encryption(const std::string& xname, unsigned xblock_size, unsigned xkey_size, unsigned xiv_size)
		: name_(xname),
		  block_size_(xblock_size),
		  key_size_(xkey_size),
		  iv_size_(xiv_size)
		{ }

	public:
		virtual ~Encryption()
		{ }

		unsigned block_size(void) const
		{
			return (block_size_);
		}

		unsigned key_size(void) const
		{
			return (key_size_);
		}

		unsigned iv_size(void) const
		{
			return (iv_size_);
		}

		std::string name(void) const
		{
			return (name_);
		}

		virtual Encryption *clone(void) const = 0;

		virtual bool initialize(CryptoEncryption::Operation, const Buffer *, const Buffer *) = 0;
		virtual bool cipher(Buffer *, Buffer *) = 0;

		static void add_algorithms(Session *);
		static Encryption *cipher(CryptoEncryption::Cipher);
	};
}

#endif /* !SSH_SSH_ENCRYPTION_H */
