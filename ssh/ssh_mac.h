#ifndef	SSH_SSH_MAC_H
#define	SSH_SSH_MAC_H

#include <crypto/crypto_mac.h>

namespace SSH {
	class MAC {
		std::string name_;
		unsigned size_;
		unsigned key_size_;
	protected:
		MAC(const std::string& xname, unsigned xsize, unsigned xkey_size)
		: name_(xname),
		  size_(xsize),
		  key_size_(xkey_size)
		{ }

	public:
		virtual ~MAC()
		{ }

		std::string name(void) const
		{
			return (name_);
		}

		unsigned size(void) const
		{
			return (size_);
		}

		unsigned key_size(void) const
		{
			return (key_size_);
		}

		virtual bool input(Buffer *) = 0;

		static MAC *algorithm(CryptoMAC::Algorithm);
	};
}

#endif /* !SSH_SSH_MAC_H */
