#include <openssl/evp.h>

#include <map>

#include <crypto/crypto_encryption.h>

class CryptoEncryptionSessionOpenSSL : public CryptoEncryptionSession {
	LogHandle log_;
	const EVP_CIPHER *cipher_;
	EVP_CIPHER_CTX ctx_;
public:
	CryptoEncryptionSessionOpenSSL(const CryptoEncryptionMethod *method, const EVP_CIPHER *cipher)
	: CryptoEncryptionSession(method),
	  log_("/crypto/encryption/session/openssl"),
	  cipher_(cipher),
	  ctx_()
	{
		EVP_CIPHER_CTX_init(&ctx_);
	}

	~CryptoEncryptionSessionOpenSSL()
	{
		EVP_CIPHER_CTX_cleanup(&ctx_);
	}

	bool initialize(CryptoEncryptionOperation operation, const Buffer *key, const Buffer *iv)
	{
		if (key->length() < (size_t)EVP_CIPHER_key_length(cipher_))
			return (false);

		if (iv->length() < (size_t)EVP_CIPHER_iv_length(cipher_))
			return (false);

		uint8_t keydata[key->length()];
		key->copyout(keydata, sizeof keydata);

		uint8_t ivdata[iv->length()];
		iv->copyout(ivdata, sizeof ivdata);

		int rv = EVP_CipherInit(&ctx_, cipher_, keydata, ivdata, operation == CryptoEncrypt ? 1 : 0);
		if (rv == 0)
			return (false);

		return (true);
	}

	Action *submit(Buffer *in, EventCallback *cb)
	{
		uint8_t indata[in->length()];
		in->moveout(indata, sizeof indata);

		uint8_t outdata[sizeof indata];
		int rv = EVP_Cipher(&ctx_, outdata, indata, sizeof indata);
		if (rv == 0) {
			cb->param(Event::Error);
			return (cb->schedule());
		}
		cb->param(Event(Event::Done, Buffer(outdata, sizeof outdata)));
		return (cb->schedule());
	}
};

class CryptoEncryptionMethodOpenSSL : public CryptoEncryptionMethod {
	LogHandle log_;
	std::map<CryptoCipher, const EVP_CIPHER *> cipher_map_;
public:
	CryptoEncryptionMethodOpenSSL(void)
	: log_("/crypto/encryption/openssl")
	{
		OpenSSL_add_all_algorithms();

		cipher_map_[CryptoCipher(CryptoAES128, CryptoModeCBC)] = EVP_aes_128_cbc();
		cipher_map_[CryptoCipher(CryptoAES192, CryptoModeCBC)] = EVP_aes_192_cbc();
		cipher_map_[CryptoCipher(CryptoAES256, CryptoModeCBC)] = EVP_aes_256_cbc();

#if 0
		std::map<CryptoCipher, const EVP_CIPHER *>::const_iterator it;
		for (it = cipher_map_.begin(); it != cipher_map_.end(); ++it)
			CryptoSystem::instance()->register_method(it->first, this);
#endif
	}

	~CryptoEncryptionMethodOpenSSL()
	{
		/* XXX Unregister.  */
	}

	CryptoEncryptionSession *session(CryptoCipher cipher) const
	{
		std::map<CryptoCipher, const EVP_CIPHER *>::const_iterator it;

		it = cipher_map_.find(cipher);
		if (it == cipher_map_.end())
			return (NULL);

		return (new CryptoEncryptionSessionOpenSSL(this, it->second));
	}
};

static CryptoEncryptionMethodOpenSSL crypto_encryption_method_openssl;
const CryptoEncryptionMethod *CryptoEncryptionMethod::default_method = &crypto_encryption_method_openssl; /* XXX */
