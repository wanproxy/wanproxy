#include <crypto/crypto_encryption.h>

namespace {
	class Registrar {
		std::set<const CryptoEncryptionMethod *> method_set_;

		Registrar(void)
		: method_set_()
		{ }

		~Registrar()
		{ }
	public:
		void enter(const CryptoEncryptionMethod *method)
		{
			method_set_.insert(method);
		}

		std::set<const CryptoEncryptionMethod *> enumerate(void) const
		{
			return (method_set_);
		}

		static Registrar *instance(void)
		{
			static Registrar *instance;

			if (instance == NULL)
				instance = new Registrar();
			return (instance);
		}
	};
};

CryptoEncryptionMethod::CryptoEncryptionMethod(const std::string& name)
: name_(name)
{
	Registrar::instance()->enter(this);
}

const CryptoEncryptionMethod *
CryptoEncryptionMethod::method(CryptoCipher cipher)
{
	std::set<const CryptoEncryptionMethod *> method_set = Registrar::instance()->enumerate();
	std::set<const CryptoEncryptionMethod *>::const_iterator it;

	for (it = method_set.begin(); it != method_set.end(); ++it) {
		const CryptoEncryptionMethod *m = *it;
		std::set<CryptoCipher> cipher_set = m->ciphers();
		if (cipher_set.find(cipher) == cipher_set.end())
			continue;

		return (*it);
	}

	ERROR("/crypto/encryption/method") << "Could not find a method for cipher: " << cipher;

	return (NULL);
}

std::ostream&
operator<< (std::ostream& os, CryptoEncryptionAlgorithm algorithm)
{
	switch (algorithm) {
	case Crypto3DES:
		return (os << "3DES");
	case CryptoAES128:
		return (os << "AES128");
	case CryptoAES192:
		return (os << "AES192");
	case CryptoAES256:
		return (os << "AES256");
	}
	NOTREACHED("/crypto/encryption");
}

std::ostream&
operator<< (std::ostream& os, CryptoEncryptionMode mode)
{
	switch (mode) {
	case CryptoModeCBC:
		return (os << "CBC");
	case CryptoModeCTR:
		return (os << "CTR");
	}
	NOTREACHED("/crypto/encryption");
}

std::ostream&
operator<< (std::ostream& os, CryptoCipher cipher)
{
	return (os << cipher.first << "/" << cipher.second);
}
