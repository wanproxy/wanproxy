#ifndef	SSH_SSH_ENCRYPTION_H
#define	SSH_SSH_ENCRYPTION_H

#include <crypto/crypto_encryption.h>

namespace SSH {
	class Encryption {
		std::string name_;
		unsigned block_size_;
		unsigned key_size_;
		unsigned iv_size_;
	protected:
		Encryption(const std::string& xname, unsigned xblock_size, unsigned xkey_size, unsigned xiv_size)
		: name_(xname),
		  block_size_(xblock_size),
		  key_size_(xkey_size),
		  iv_size_(xiv_size)
		{ }

	public:
		virtual ~Encryption()
		{ }

		unsigned block_size(void) const
		{
			return (block_size_);
		}

		unsigned key_size(void) const
		{
			return (key_size_);
		}

		unsigned iv_size(void) const
		{
			return (iv_size_);
		}

		std::string name(void) const
		{
			return (name_);
		}

		virtual bool input(Buffer *) = 0;

		static Encryption *cipher(CryptoEncryption::Cipher);
	};
}

#endif /* !SSH_SSH_ENCRYPTION_H */
