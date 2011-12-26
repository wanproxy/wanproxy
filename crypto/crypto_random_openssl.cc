#include <openssl/evp.h>
#include <openssl/rand.h>

#include <common/factory.h>

#include <crypto/crypto_random.h>

namespace {
	typedef	int (RAND_func)(unsigned char *, int);
}

class CryptoRandomSessionRAND : public CryptoRandomSession {
	LogHandle log_;
	RAND_func *func_;
public:
	CryptoRandomSessionRAND(RAND_func *func)
	: log_("/crypto/random/session/openssl"),
	  func_(func)
	{ }

	~CryptoRandomSessionRAND()
	{ }

	Action *generate(size_t len, EventCallback *cb)
	{
		ASSERT(log_, len != 0);

		/*
		 * We process a single, large, linear byte buffer here rather
		 * than going a BufferSegment at a time, even though the byte
		 * buffer is less efficient than some alternatives, because
		 * I am a bad person and I should feel bad.
		 */
		uint8_t bytes[len];
		int rv = func_(bytes, sizeof bytes);
		if (rv == 0) {
			cb->param(Event::Error);
			return (cb->schedule());
		}
		cb->param(Event(Event::Done, Buffer(bytes, sizeof bytes)));
		return (cb->schedule());
	}
};

class CryptoRandomMethodOpenSSL : public CryptoRandomMethod {
	LogHandle log_;
	std::map<CryptoRandomType, RAND_func *> func_map_;
public:
	CryptoRandomMethodOpenSSL(void)
	: log_("/crypto/random/openssl"),
	  func_map_()
	{
		OpenSSL_add_all_algorithms();

		func_map_[CryptoTypeRNG] = RAND_bytes;
		func_map_[CryptoTypePRNG] = RAND_pseudo_bytes;

		/* XXX Register.  */
	}

	~CryptoRandomMethodOpenSSL()
	{
		/* XXX Unregister.  */
	}

	/*
	 * Synchronous randomness generation.  May not succeed.
	 */
	bool generate(CryptoRandomType func, size_t len, Buffer *out) const
	{
		std::map<CryptoRandomType, RAND_func *>::const_iterator it;

		it = func_map_.find(func);
		if (it == func_map_.end())
			return (false);

		uint8_t bytes[len];
		int rv = it->second(bytes, sizeof bytes);
		if (rv == 0)
			return (false);

		out->append(bytes, sizeof bytes);
		return (true);
	}

	CryptoRandomSession *session(CryptoRandomType func) const
	{
		std::map<CryptoRandomType, RAND_func *>::const_iterator it;

		it = func_map_.find(func);
		if (it != func_map_.end())
			return (new CryptoRandomSessionRAND(it->second));
		return (NULL);
	}
};

namespace {
	static CryptoRandomMethodOpenSSL crypto_random_method_openssl;
}
const CryptoRandomMethod *CryptoRandomMethod::default_method = &crypto_random_method_openssl; /* XXX */
