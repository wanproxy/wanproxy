#include <ssh/ssh_encryption.h>

namespace {
	struct ssh_encryption_algorithm {
		const char *rfc4250_name_;
		CryptoEncryption::Algorithm crypto_algorithm_;
		CryptoEncryption::Mode crypto_mode_;
	};

	static const struct ssh_encryption_algorithm ssh_encryption_algorithms[] = {
		{ "3des-cbc",		CryptoEncryption::TripleDES,	CryptoEncryption::CBC	},
		{ "aes128-cbc",		CryptoEncryption::AES128,	CryptoEncryption::CBC	},
		{ "aes128-ctr",		CryptoEncryption::AES128,	CryptoEncryption::CTR	},
		{ "aes192-cbc",		CryptoEncryption::AES192,	CryptoEncryption::CBC	},
		{ "aes192-ctr",		CryptoEncryption::AES192,	CryptoEncryption::CTR	},
		{ "aes256-cbc",		CryptoEncryption::AES256,	CryptoEncryption::CBC	},
		{ "aes256-ctr",		CryptoEncryption::AES256,	CryptoEncryption::CTR	},
		{ NULL,			CryptoEncryption::AES128,	CryptoEncryption::CBC	}
	};

	class CryptoSSHEncryption : public SSH::Encryption {
		LogHandle log_;
		const CryptoEncryption::Method *method_;
	public:
		CryptoSSHEncryption(const std::string& xname, const CryptoEncryption::Method *method)
		: SSH::Encryption(xname),
		  log_("/ssh/encryption/crypto/" + xname),
		  method_(method)
		{
			ASSERT(log_, method_ != NULL);
		}

		~CryptoSSHEncryption()
		{ }

		bool input(Buffer *)
		{
			ERROR(log_) << "Not yet implemented.";
			return (false);
		}
	};
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
		if (method == NULL)
			return (NULL);
		return (new CryptoSSHEncryption(alg->rfc4250_name_, method));
	}
	return (NULL);
}
