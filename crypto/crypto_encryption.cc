#include <common/registrar.h>

#include <crypto/crypto_encryption.h>

namespace {
	struct CryptoEncryptionMethodRegistrarKey;
	typedef	Registrar<CryptoEncryptionMethodRegistrarKey, const CryptoEncryptionMethod *> CryptoEncryptionMethodRegistrar;
}

CryptoEncryptionMethod::CryptoEncryptionMethod(const std::string& name)
: name_(name)
{
	CryptoEncryptionMethodRegistrar::instance()->enter(this);
}

const CryptoEncryptionMethod *
CryptoEncryptionMethod::method(CryptoCipher cipher)
{
	std::set<const CryptoEncryptionMethod *> method_set = CryptoEncryptionMethodRegistrar::instance()->enumerate();
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
