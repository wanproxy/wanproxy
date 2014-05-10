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

#include <crypto/crypto_mac.h>

namespace {
	struct MethodRegistrarKey;
	typedef	Registrar<MethodRegistrarKey, const CryptoMAC::Method *> MethodRegistrar;
}

CryptoMAC::Method::Method(const std::string& name)
: name_(name)
{
	MethodRegistrar::instance()->enter(this);
}

const CryptoMAC::Method *
CryptoMAC::Method::method(CryptoMAC::Algorithm algorithm)
{
	std::set<const CryptoMAC::Method *> method_set = MethodRegistrar::instance()->enumerate();
	std::set<const CryptoMAC::Method *>::const_iterator it;

	for (it = method_set.begin(); it != method_set.end(); ++it) {
		const CryptoMAC::Method *m = *it;
		std::set<CryptoMAC::Algorithm> algorithm_set = m->algorithms();
		if (algorithm_set.find(algorithm) == algorithm_set.end())
			continue;

		return (*it);
	}

	ERROR("/crypto/encryption/method") << "Could not find a method for algorithm: " << algorithm;

	return (NULL);
}

std::ostream&
operator<< (std::ostream& os, CryptoMAC::Algorithm algorithm)
{
	switch (algorithm) {
	case CryptoMAC::MD5:
		return (os << "HMAC-MD5");
	case CryptoMAC::SHA1:
		return (os << "HMAC-SHA1");
	case CryptoMAC::SHA256:
		return (os << "HMAC-SHA256");
	case CryptoMAC::SHA512:
		return (os << "HMAC-SHA512");
	case CryptoMAC::RIPEMD160:
		return (os << "HMAC-RIPEMD160");
	}
	NOTREACHED("/crypto/encryption");
}
