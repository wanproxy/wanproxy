#include <ssh/ssh_mac.h>

namespace {
	struct ssh_mac_algorithm {
		const char *rfc4250_name_;
		CryptoMAC::Algorithm crypto_algorithm_;
		unsigned size_;
	};

	static const struct ssh_mac_algorithm ssh_mac_algorithms[] = {
		{ "hmac-md5",		CryptoMAC::MD5,		0 },
		{ "hmac-md5-96",	CryptoMAC::MD5,		12 },
		{ "hmac-sha1",		CryptoMAC::SHA1,	0 },
		{ "hmac-sha1-96",	CryptoMAC::SHA1,	12 },
		{ "hmac-sha256",	CryptoMAC::SHA256,	0 },
		{ NULL,			CryptoMAC::MD5,		0 }
	};

	class CryptoSSHMAC : public SSH::MAC {
		LogHandle log_;
		CryptoMAC::Instance *instance_;
	public:
		CryptoSSHMAC(const std::string& xname, CryptoMAC::Instance *instance, unsigned xsize)
		: SSH::MAC(xname, xsize == 0 ? instance->size() : xsize, instance->size()),
		  log_("/ssh/mac/crypto/" + xname),
		  instance_(instance)
		{ }

		~CryptoSSHMAC()
		{ }

		MAC *clone(void) const
		{
			return (new CryptoSSHMAC(name_, instance_->clone(), key_size_));
		}

		bool initialize(const Buffer *key)
		{
			return (instance_->initialize(key));
		}

		bool mac(Buffer *out, const Buffer *in)
		{
			return (instance_->mac(out, in));
		}
	};
}

SSH::MAC *
SSH::MAC::algorithm(CryptoMAC::Algorithm xalgorithm)
{
	const struct ssh_mac_algorithm *alg;

	for (alg = ssh_mac_algorithms; alg->rfc4250_name_ != NULL; alg++) {
		if (xalgorithm != alg->crypto_algorithm_)
			continue;
		const CryptoMAC::Method *method = CryptoMAC::Method::method(xalgorithm);
		if (method == NULL) {
			ERROR("/ssh/mac") << "Could not get method for algorithm: " << xalgorithm;
			return (NULL);
		}
		CryptoMAC::Instance *instance = method->instance(xalgorithm);
		if (instance == NULL) {
			ERROR("/ssh/mac") << "Could not get instance for algorithm: " << xalgorithm;
			return (NULL);
		}
		return (new CryptoSSHMAC(alg->rfc4250_name_, instance, alg->size_));
	}
	ERROR("/ssh/mac") << "No SSH MAC support is available for algorithm: " << xalgorithm;
	return (NULL);
}
