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

#include <common/registrar.h>

#include <crypto/crypto_encryption.h>

namespace {
	struct MethodRegistrarKey;
	typedef	Registrar<MethodRegistrarKey, const CryptoEncryption::Method *> MethodRegistrar;
}

CryptoEncryption::Method::Method(const std::string& name)
: name_(name)
{
	MethodRegistrar::instance()->enter(this);
}

const CryptoEncryption::Method *
CryptoEncryption::Method::method(CryptoEncryption::Cipher cipher)
{
	std::set<const CryptoEncryption::Method *> method_set = MethodRegistrar::instance()->enumerate();
	std::set<const CryptoEncryption::Method *>::const_iterator it;

	for (it = method_set.begin(); it != method_set.end(); ++it) {
		const CryptoEncryption::Method *m = *it;
		std::set<CryptoEncryption::Cipher> cipher_set = m->ciphers();
		if (cipher_set.find(cipher) == cipher_set.end())
			continue;

		return (*it);
	}

	ERROR("/crypto/encryption/method") << "Could not find a method for cipher: " << cipher;

	return (NULL);
}

std::ostream&
operator<< (std::ostream& os, CryptoEncryption::Algorithm algorithm)
{
	switch (algorithm) {
	case CryptoEncryption::TripleDES:
		return (os << "3DES");
	case CryptoEncryption::AES128:
		return (os << "AES128");
	case CryptoEncryption::AES192:
		return (os << "AES192");
	case CryptoEncryption::AES256:
		return (os << "AES256");
	case CryptoEncryption::Blowfish:
		return (os << "Blowfish");
	case CryptoEncryption::CAST:
		return (os << "CAST");
	case CryptoEncryption::IDEA:
		return (os << "IDEA");
	case CryptoEncryption::RC4:
		return (os << "RC4");
	}
	NOTREACHED("/crypto/encryption");
}

std::ostream&
operator<< (std::ostream& os, CryptoEncryption::Mode mode)
{
	switch (mode) {
	case CryptoEncryption::CBC:
		return (os << "CBC");
	case CryptoEncryption::CTR:
		return (os << "CTR");
	case CryptoEncryption::Stream:
		return (os << "Stream");
	}
	NOTREACHED("/crypto/encryption");
}

std::ostream&
operator<< (std::ostream& os, CryptoEncryption::Cipher cipher)
{
	return (os << cipher.first << "/" << cipher.second);
}
