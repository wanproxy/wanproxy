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
