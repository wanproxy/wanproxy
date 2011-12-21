#include <ssh/ssh_encryption.h>

namespace {
	struct ssh_encryption_algorithm {
		const char *rfc4250_name_;
		CryptoEncryptionAlgorithm crypto_algorithm_;
		CryptoEncryptionMode crypto_mode_;
	};

	static const struct ssh_encryption_algorithm ssh_encryption_algorithms[] = {
		{ "aes128-cbc",		CryptoAES128,	CryptoModeCBC	},
		{ NULL,			CryptoAES128,	CryptoModeCBC	}
	};

	class CryptoEncryption: public SSH::Encryption {
		LogHandle log_;
		const CryptoEncryptionMethod *method_;
	public:
		CryptoEncryption(const std::string& xname, const CryptoEncryptionMethod *method)
		: SSH::Encryption(xname),
		  log_("/ssh/encryption/crypto/" + xname),
		  method_(method)
		{
			ASSERT(log_, method_ != NULL);
		}

		~CryptoEncryption()
		{ }

		bool input(Buffer *)
		{
			ERROR(log_) << "Not yet implemented.";
			return (false);
		}
	};
}

SSH::Encryption *
SSH::Encryption::cipher(CryptoCipher cipher)
{
	const struct ssh_encryption_algorithm *alg;

	for (alg = ssh_encryption_algorithms; alg->rfc4250_name_ != NULL; alg++) {
		if (cipher.first != alg->crypto_algorithm_)
			continue;
		if (cipher.second != alg->crypto_mode_)
			continue;
		const CryptoEncryptionMethod *method = CryptoEncryptionMethod::method(cipher);
		if (method == NULL)
			return (NULL);
		return (new CryptoEncryption(alg->rfc4250_name_, method));
	}
	return (NULL);
}
