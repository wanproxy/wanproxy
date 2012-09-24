#include <ssh/ssh_mac.h>

namespace {
	struct ssh_mac_algorithm {
		const char *rfc4250_name_;
		CryptoMAC::Algorithm crypto_algorithm_;
	};

	static const struct ssh_mac_algorithm ssh_mac_algorithms[] = {
		{ "hmac-md5",		CryptoMAC::MD5	},
		{ "hmac-sha1",		CryptoMAC::SHA1 },
		{ "hmac-sha256",	CryptoMAC::SHA256 },
		{ NULL,			CryptoMAC::MD5	}
	};

	class CryptoSSHMAC : public SSH::MAC {
		LogHandle log_;
		const CryptoMAC::Method *method_;
	public:
		CryptoSSHMAC(const std::string& xname, const CryptoMAC::Method *method)
		: SSH::MAC(xname),
		  log_("/ssh/mac/crypto/" + xname),
		  method_(method)
		{
			ASSERT(log_, method_ != NULL);
		}

		~CryptoSSHMAC()
		{ }

		bool input(Buffer *)
		{
			ERROR(log_) << "Not yet implemented.";
			return (false);
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
		return (new CryptoSSHMAC(alg->rfc4250_name_, method));
	}
	ERROR("/ssh/mac") << "No SSH MAC support is available for algorithm: " << xalgorithm;
	return (NULL);
}
