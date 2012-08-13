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
		return (os << "MD5");
	}
	NOTREACHED("/crypto/encryption");
}
