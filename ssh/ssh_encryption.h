#ifndef	SSH_SSH_ENCRYPTION_H
#define	SSH_SSH_ENCRYPTION_H

#include <crypto/crypto_encryption.h>

namespace SSH {
	struct Session;

	class Encryption {
	protected:
		const std::string name_;
		const unsigned block_size_;
		const unsigned key_size_;
		const unsigned iv_size_;

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

		virtual Encryption *clone(void) const = 0;

		virtual bool initialize(CryptoEncryption::Operation, const Buffer *, const Buffer *) = 0;
		virtual bool cipher(Buffer *, Buffer *) = 0;

		static void add_algorithms(Session *);
		static Encryption *cipher(CryptoEncryption::Cipher);
	};
}

#endif /* !SSH_SSH_ENCRYPTION_H */
