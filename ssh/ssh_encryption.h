#ifndef	SSH_SSH_ENCRYPTION_H
#define	SSH_SSH_ENCRYPTION_H

#include <crypto/crypto_encryption.h>

namespace SSH {
	class Encryption {
		std::string name_;
	protected:
		Encryption(const std::string& xname)
		: name_(xname)
		{ }

	public:
		virtual ~Encryption()
		{ }

		std::string name(void) const
		{
			return (name_);
		}

		virtual bool input(Buffer *) = 0;

		static Encryption *cipher(CryptoEncryption::Cipher);
	};
}

#endif /* !SSH_SSH_ENCRYPTION_H */
