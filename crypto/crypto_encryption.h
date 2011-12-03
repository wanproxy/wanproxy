#ifndef	CRYPTO_ENCRYPTION_H
#define	CRYPTO_ENCRYPTION_H

#include <event/event_callback.h>

class CryptoEncryptionMethod;

enum CryptoEncryptionAlgorithm {
	Crypto3DES,
	CryptoAES128,
	CryptoAES192,
	CryptoAES256,
};

enum CryptoEncryptionMode {
	CryptoModeCBC,
	CryptoModeCTR,
};
typedef	std::pair<CryptoEncryptionAlgorithm, CryptoEncryptionMode> CryptoCipher;

enum CryptoEncryptionOperation {
	CryptoEncrypt,
	CryptoDecrypt,
};

class CryptoEncryptionSession {
protected:
	CryptoEncryptionSession(void)
	{ }

public:
	virtual ~CryptoEncryptionSession()
	{ }

	virtual bool initialize(CryptoEncryptionOperation, const Buffer *, const Buffer *) = 0;
	virtual Action *submit(Buffer *, EventCallback *) = 0;
};

class CryptoEncryptionMethod {
protected:
	CryptoEncryptionMethod(void)
	{ }

	virtual ~CryptoEncryptionMethod()
	{ }
public:
	virtual CryptoEncryptionSession *session(CryptoCipher) const = 0;

	/* XXX Registration API somehow.  */
	static const CryptoEncryptionMethod *default_method;
};

#endif /* !CRYPTO_ENCRYPTION_H */
