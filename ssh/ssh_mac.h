#ifndef	SSH_SSH_MAC_H
#define	SSH_SSH_MAC_H

#include <crypto/crypto_mac.h>

namespace SSH {
	struct Session;

	class MAC {
	protected:
		const std::string name_;
		const unsigned size_;
		const unsigned key_size_;

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

		virtual MAC *clone(void) const = 0;

		virtual bool initialize(const Buffer *) = 0;
		virtual bool mac(Buffer *, const Buffer *) = 0;

		static void add_algorithms(Session *);
		static MAC *algorithm(CryptoMAC::Algorithm);
	};
}

#endif /* !SSH_SSH_MAC_H */
