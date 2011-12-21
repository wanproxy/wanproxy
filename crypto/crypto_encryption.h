#ifndef	CRYPTO_CRYPTO_ENCRYPTION_H
#define	CRYPTO_CRYPTO_ENCRYPTION_H

#include <set>

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
	std::string name_;
protected:
	CryptoEncryptionMethod(const std::string&);

	virtual ~CryptoEncryptionMethod()
	{ }
public:
	virtual std::set<CryptoCipher> ciphers(void) const = 0;
	virtual CryptoEncryptionSession *session(CryptoCipher) const = 0;

	static const CryptoEncryptionMethod *method(CryptoCipher);
};

std::ostream& operator<< (std::ostream&, CryptoEncryptionAlgorithm);
std::ostream& operator<< (std::ostream&, CryptoEncryptionMode);
std::ostream& operator<< (std::ostream&, CryptoCipher);

#endif /* !CRYPTO_CRYPTO_ENCRYPTION_H */
