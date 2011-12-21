#ifndef	CRYPTO_CRYPTO_RANDOM_H
#define	CRYPTO_CRYPTO_RANDOM_H

#include <event/event_callback.h>

class CryptoRandomMethod;

/*
 * XXX
 * Add a CryptoTypeBest, which will use whatever the best, available method is.
 */
enum CryptoRandomType {
	CryptoTypeRNG,
	CryptoTypePRNG,
};

class CryptoRandomSession {
protected:
	CryptoRandomSession(void)
	{ }

public:
	virtual ~CryptoRandomSession()
	{ }

	virtual Action *generate(size_t, EventCallback *) = 0;
};

class CryptoRandomMethod {
protected:
	CryptoRandomMethod(void)
	{ }

	virtual ~CryptoRandomMethod()
	{ }
public:
	virtual bool generate(CryptoRandomType, size_t, Buffer *) const = 0;
	virtual CryptoRandomSession *session(CryptoRandomType) const = 0;

	/* XXX Registration API somehow.  */
	static const CryptoRandomMethod *default_method;
};

#endif /* !CRYPTO_CRYPTO_RANDOM_H */
