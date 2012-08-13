#include <openssl/evp.h>

#include <common/factory.h>

#include <crypto/crypto_encryption.h>

namespace {
	class SessionEVP : public CryptoEncryption::Session {
		LogHandle log_;
		const EVP_CIPHER *cipher_;
		EVP_CIPHER_CTX ctx_;
	public:
		SessionEVP(const EVP_CIPHER *cipher)
		: log_("/crypto/encryption/session/openssl"),
		  cipher_(cipher),
		  ctx_()
		{
			EVP_CIPHER_CTX_init(&ctx_);
		}

		~SessionEVP()
		{
			EVP_CIPHER_CTX_cleanup(&ctx_);
		}

		bool initialize(CryptoEncryption::Operation operation, const Buffer *key, const Buffer *iv)
		{
			if (key->length() < (size_t)EVP_CIPHER_key_length(cipher_))
				return (false);

			if (iv->length() < (size_t)EVP_CIPHER_iv_length(cipher_))
				return (false);

			int enc;
			switch (operation) {
			case CryptoEncryption::Encrypt:
				enc = 1;
				break;
			case CryptoEncryption::Decrypt:
				enc = 0;
				break;
			default:
				return (false);
			}

			uint8_t keydata[key->length()];
			key->copyout(keydata, sizeof keydata);

			uint8_t ivdata[iv->length()];
			iv->copyout(ivdata, sizeof ivdata);

			int rv = EVP_CipherInit(&ctx_, cipher_, keydata, ivdata, enc);
			if (rv == 0)
				return (false);

			return (true);
		}

		Action *submit(Buffer *in, EventCallback *cb)
		{
			/*
			 * We process a single, large, linear byte buffer here rather
			 * than going a BufferSegment at a time, even though the byte
			 * buffer is less efficient than some alternatives, because
			 * there are padding and buffering implications if each
			 * BufferSegment's length is not modular to the block size.
			 */
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

	class MethodOpenSSL : public CryptoEncryption::Method {
		LogHandle log_;
		FactoryMap<CryptoEncryption::Cipher, CryptoEncryption::Session> cipher_map_;
	public:
		MethodOpenSSL(void)
		: CryptoEncryption::Method("OpenSSL"),
		  log_("/crypto/encryption/openssl"),
		  cipher_map_()
		{
			OpenSSL_add_all_algorithms();

			factory<SessionEVP> evp_factory;
			cipher_map_.enter(CryptoEncryption::Cipher(CryptoEncryption::TripleDES, CryptoEncryption::CBC), evp_factory(EVP_des_ede3_cbc()));
			cipher_map_.enter(CryptoEncryption::Cipher(CryptoEncryption::AES128, CryptoEncryption::CBC), evp_factory(EVP_aes_128_cbc()));
			cipher_map_.enter(CryptoEncryption::Cipher(CryptoEncryption::AES192, CryptoEncryption::CBC), evp_factory(EVP_aes_192_cbc()));
			cipher_map_.enter(CryptoEncryption::Cipher(CryptoEncryption::AES256, CryptoEncryption::CBC), evp_factory(EVP_aes_256_cbc()));

			/* XXX Register.  */
		}

		~MethodOpenSSL()
		{
			/* XXX Unregister.  */
		}

		std::set<CryptoEncryption::Cipher> ciphers(void) const
		{
			return (cipher_map_.keys());
		}

		CryptoEncryption::Session *session(CryptoEncryption::Cipher cipher) const
		{
			return (cipher_map_.create(cipher));
		}
	};

	static MethodOpenSSL crypto_encryption_method_openssl;
}
