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

#include <common/buffer.h>

#include <ssh/ssh_algorithm_negotiation.h>
#include <ssh/ssh_encryption.h>
#include <ssh/ssh_session.h>

namespace {
	struct ssh_encryption_algorithm {
		const char *rfc4250_name_;
		CryptoEncryption::Algorithm crypto_algorithm_;
		CryptoEncryption::Mode crypto_mode_;
	};

	static const struct ssh_encryption_algorithm ssh_encryption_algorithms[] = {
		{ "aes128-ctr",		CryptoEncryption::AES128,	CryptoEncryption::CTR	},
		{ "aes128-cbc",		CryptoEncryption::AES128,	CryptoEncryption::CBC	},
		{ "aes192-ctr",		CryptoEncryption::AES192,	CryptoEncryption::CTR	},
		{ "aes192-cbc",		CryptoEncryption::AES192,	CryptoEncryption::CBC	},
		{ "aes256-ctr",		CryptoEncryption::AES256,	CryptoEncryption::CTR	},
		{ "aes256-cbc",		CryptoEncryption::AES256,	CryptoEncryption::CBC	},
		{ "blowfish-ctr",	CryptoEncryption::Blowfish,	CryptoEncryption::CTR	},
		{ "blowfish-cbc",	CryptoEncryption::Blowfish,	CryptoEncryption::CBC	},
		{ "3des-ctr",		CryptoEncryption::TripleDES,	CryptoEncryption::CTR	},
		{ "3des-cbc",		CryptoEncryption::TripleDES,	CryptoEncryption::CBC	},
		{ "cast128-cbc",	CryptoEncryption::CAST,		CryptoEncryption::CBC	},
		{ "idea-cbc",		CryptoEncryption::IDEA,		CryptoEncryption::CBC	},
		{ "arcfour",		CryptoEncryption::RC4,		CryptoEncryption::Stream},
		{ NULL,			CryptoEncryption::AES128,	CryptoEncryption::CBC	}
	};

	class CryptoSSHEncryption : public SSH::Encryption {
		LogHandle log_;
		CryptoEncryption::Session *session_;
	public:
		CryptoSSHEncryption(const std::string& xname, CryptoEncryption::Session *session)
		: SSH::Encryption(xname, session->block_size(), session->key_size(), session->iv_size()),
		  log_("/ssh/encryption/crypto/" + xname),
		  session_(session)
		{ }

		~CryptoSSHEncryption()
		{ }

		Encryption *clone(void) const
		{
			return (new CryptoSSHEncryption(name_, session_->clone()));
		}

		bool initialize(CryptoEncryption::Operation operation, const Buffer *key, const Buffer *iv)
		{
			return (session_->initialize(operation, key, iv));
		}

		bool cipher(Buffer *out, Buffer *in)
		{
			if (!session_->cipher(out, in)) {
				in->clear();
				return (false);
			}
			in->clear();
			return (true);
		}
	};
}

void
SSH::Encryption::add_algorithms(Session *session)
{
	const struct ssh_encryption_algorithm *alg;

	for (alg = ssh_encryption_algorithms; alg->rfc4250_name_ != NULL; alg++) {
		Encryption *encryption = cipher(CryptoEncryption::Cipher(alg->crypto_algorithm_, alg->crypto_mode_));
		if (encryption == NULL)
			continue;
		session->algorithm_negotiation_->add_algorithm(encryption);
	}
}

SSH::Encryption *
SSH::Encryption::cipher(CryptoEncryption::Cipher cipher)
{
	const struct ssh_encryption_algorithm *alg;

	for (alg = ssh_encryption_algorithms; alg->rfc4250_name_ != NULL; alg++) {
		if (cipher.first != alg->crypto_algorithm_)
			continue;
		if (cipher.second != alg->crypto_mode_)
			continue;
		const CryptoEncryption::Method *method = CryptoEncryption::Method::method(cipher);
		if (method == NULL) {
			DEBUG("/ssh/encryption") << "Could not get method for cipher: " << cipher;
			return (NULL);
		}
		CryptoEncryption::Session *session = method->session(cipher);
		if (session == NULL) {
			ERROR("/ssh/encryption") << "Could not get session for cipher: " << cipher;
			return (NULL);
		}
		return (new CryptoSSHEncryption(alg->rfc4250_name_, session));
	}
	DEBUG("/ssh/encryption") << "No SSH encryption support is available for cipher: " << cipher;
	return (NULL);
}
