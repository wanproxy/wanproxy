#ifndef	CRYPTO_CRYPTO_ENCRYPTION_H
#define	CRYPTO_CRYPTO_ENCRYPTION_H

#include <set>

#include <event/event_callback.h>

namespace CryptoEncryption {
	class Method;

	enum Algorithm {
		TripleDES,
		AES128,
		AES192,
		AES256,
		Blowfish,
		CAST,
		IDEA,
		RC4,
	};

	enum Mode {
		CBC,
		CTR,
		Stream,
	};
	typedef	std::pair<Algorithm, Mode> Cipher;

	enum Operation {
		Encrypt,
		Decrypt,
	};

	class Session {
	protected:
		Session(void)
		{ }

	public:
		virtual ~Session()
		{ }

		virtual unsigned block_size(void) const = 0;
		virtual unsigned key_size(void) const = 0;
		virtual unsigned iv_size(void) const = 0;

		virtual Session *clone(void) const = 0;

		virtual bool initialize(Operation, const Buffer *, const Buffer *) = 0;

		virtual bool cipher(Buffer *, const Buffer *) = 0;

		virtual Action *submit(Buffer *, EventCallback *) = 0;
	};

	class Method {
		std::string name_;
	protected:
		Method(const std::string&);

		virtual ~Method()
		{ }
	public:
		virtual std::set<Cipher> ciphers(void) const = 0;
		virtual Session *session(Cipher) const = 0;

		static const Method *method(Cipher);
	};
}

std::ostream& operator<< (std::ostream&, CryptoEncryption::Algorithm);
std::ostream& operator<< (std::ostream&, CryptoEncryption::Mode);
std::ostream& operator<< (std::ostream&, CryptoEncryption::Cipher);

#endif /* !CRYPTO_CRYPTO_ENCRYPTION_H */
