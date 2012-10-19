#include <common/registrar.h>

#include <crypto/crypto_hash.h>

namespace {
	struct MethodRegistrarKey;
	typedef	Registrar<MethodRegistrarKey, const CryptoHash::Method *> MethodRegistrar;
}

CryptoHash::Method::Method(const std::string& name)
: name_(name)
{
	MethodRegistrar::instance()->enter(this);
}

const CryptoHash::Method *
CryptoHash::Method::method(CryptoHash::Algorithm algorithm)
{
	std::set<const CryptoHash::Method *> method_set = MethodRegistrar::instance()->enumerate();
	std::set<const CryptoHash::Method *>::const_iterator it;

	for (it = method_set.begin(); it != method_set.end(); ++it) {
		const CryptoHash::Method *m = *it;
		std::set<CryptoHash::Algorithm> algorithm_set = m->algorithms();
		if (algorithm_set.find(algorithm) == algorithm_set.end())
			continue;

		return (*it);
	}

	ERROR("/crypto/encryption/method") << "Could not find a method for algorithm: " << algorithm;

	return (NULL);
}

std::ostream&
operator<< (std::ostream& os, CryptoHash::Algorithm algorithm)
{
	switch (algorithm) {
	case CryptoHash::MD5:
		return (os << "MD5");
	case CryptoHash::SHA1:
		return (os << "SHA1");
	case CryptoHash::SHA256:
		return (os << "SHA256");
	case CryptoHash::SHA512:
		return (os << "SHA512");
	case CryptoHash::RIPEMD160:
		return (os << "RIPEMD160");
	}
	NOTREACHED("/crypto/encryption");
}
