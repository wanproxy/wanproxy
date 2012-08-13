#ifndef	SSH_SSH_MAC_H
#define	SSH_SSH_MAC_H

#include <crypto/crypto_mac.h>

namespace SSH {
	class MAC {
		std::string name_;
	protected:
		MAC(const std::string& xname)
		: name_(xname)
		{ }

	public:
		virtual ~MAC()
		{ }

		std::string name(void) const
		{
			return (name_);
		}

		virtual bool input(Buffer *) = 0;

		static MAC *algorithm(CryptoMAC::Algorithm);
	};
}

#endif /* !SSH_SSH_MAC_H */
