#include <openssl/aes.h>
#include <openssl/evp.h>

#include <common/factory.h>

#include <crypto/crypto_encryption.h>

namespace {
	class SessionEVP : public CryptoEncryption::Session {
		LogHandle log_;
		const EVP_CIPHER *cipher_;
		EVP_CIPHER_CTX ctx_;
	public:
		SessionEVP(const EVP_CIPHER *xcipher)
		: log_("/crypto/encryption/session/openssl"),
		  cipher_(xcipher),
		  ctx_()
		{
			EVP_CIPHER_CTX_init(&ctx_);
		}

		~SessionEVP()
		{
			EVP_CIPHER_CTX_cleanup(&ctx_);
		}

		unsigned block_size(void) const
		{
			return (EVP_CIPHER_block_size(cipher_));
		}

		unsigned key_size(void) const
		{
			return (EVP_CIPHER_key_length(cipher_));
		}

		unsigned iv_size(void) const
		{
			return (EVP_CIPHER_iv_length(cipher_));
		}

		Session *clone(void) const
		{
			return (new SessionEVP(cipher_));
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

		bool cipher(Buffer *out, const Buffer *in)
		{
			/*
			 * We process a single, large, linear byte buffer here rather
			 * than going a BufferSegment at a time, even though the byte
			 * buffer is less efficient than some alternatives, because
			 * there are padding and buffering implications if each
			 * BufferSegment's length is not modular to the block size.
			 */
			uint8_t indata[in->length()];
			in->copyout(indata, sizeof indata);

			uint8_t outdata[sizeof indata];
			int rv = EVP_Cipher(&ctx_, outdata, indata, sizeof indata);
			if (rv == 0)
				return (false);
			out->append(outdata, sizeof outdata);
			return (true);
		}

		Action *submit(Buffer *in, EventCallback *cb)
		{
			Buffer out;
			if (!cipher(&out, in)) {
				in->clear();
				cb->param(Event::Error);
				return (cb->schedule());
			}
			in->clear();
			cb->param(Event(Event::Done, out));
			return (cb->schedule());
		}
	};

	class SessionAES128CTR : public CryptoEncryption::Session {
		LogHandle log_;
		AES_KEY key_;
		uint8_t iv_[AES_BLOCK_SIZE];
	public:
		SessionAES128CTR(void)
		: log_("/crypto/encryption/session/openssl"),
		  key_(),
		  iv_()
		{ }

		~SessionAES128CTR()
		{ }

		unsigned block_size(void) const
		{
			return (AES_BLOCK_SIZE);
		}

		unsigned key_size(void) const
		{
			return (AES_BLOCK_SIZE);
		}

		unsigned iv_size(void) const
		{
			return (AES_BLOCK_SIZE);
		}

		Session *clone(void) const
		{
			return (new SessionAES128CTR());
		}

		bool initialize(CryptoEncryption::Operation operation, const Buffer *key, const Buffer *iv)
		{
			(void)operation;

			if (key->length() != AES_BLOCK_SIZE)
				return (false);

			if (iv->length() != AES_BLOCK_SIZE)
				return (false);

			uint8_t keydata[key->length()];
			key->copyout(keydata, sizeof keydata);

			AES_set_encrypt_key(keydata, AES_BLOCK_SIZE * 8, &key_);

			iv->copyout(iv_, sizeof iv_);

			return (true);
		}

		bool cipher(Buffer *out, const Buffer *in)
		{
			ASSERT(log_, in->length() % AES_BLOCK_SIZE == 0);

			/*
			 * Temporaries for AES_ctr128_encrypt.
			 *
			 * Their values only need to persist if we aren't using block-sized
			 * buffers, which we are.  We could just use AES_ctr128_inc and do
			 * the crypt operation by hand here.
			 */
			uint8_t counterbuf[AES_BLOCK_SIZE]; /* Will be initialized if countern==0.  */
			unsigned countern = 0;

			/*
			 * We process a single, large, linear byte buffer here rather
			 * than going a BufferSegment at a time, even though the byte
			 * buffer is less efficient than some alternatives, because
			 * there are padding and buffering implications if each
			 * BufferSegment's length is not modular to the block size.
			 */
			uint8_t indata[in->length()];
			in->copyout(indata, sizeof indata);

			uint8_t outdata[sizeof indata];
			AES_ctr128_encrypt(indata, outdata, sizeof indata, &key_, iv_, counterbuf, &countern);

			out->append(outdata, sizeof outdata);
			return (true);
		}

		Action *submit(Buffer *in, EventCallback *cb)
		{
			Buffer out;
			if (!cipher(&out, in)) {
				in->clear();
				cb->param(Event::Error);
				return (cb->schedule());
			}
			in->clear();
			cb->param(Event(Event::Done, out));
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
#if 0
			cipher_map_.enter(CryptoEncryption::Cipher(CryptoEncryption::AES128, CryptoEncryption::CTR), evp_factory(EVP_aes_128_ctr()));
			cipher_map_.enter(CryptoEncryption::Cipher(CryptoEncryption::AES192, CryptoEncryption::CTR), evp_factory(EVP_aes_192_ctr()));
			cipher_map_.enter(CryptoEncryption::Cipher(CryptoEncryption::AES256, CryptoEncryption::CTR), evp_factory(EVP_aes_256_ctr()));
#else
			factory<SessionAES128CTR> aes128ctr_factory;
			cipher_map_.enter(CryptoEncryption::Cipher(CryptoEncryption::AES128, CryptoEncryption::CTR), aes128ctr_factory());
#endif
#ifndef	OPENSSL_NO_BF
			cipher_map_.enter(CryptoEncryption::Cipher(CryptoEncryption::Blowfish, CryptoEncryption::CBC), evp_factory(EVP_bf_cbc()));
#endif
#ifndef	OPENSSL_NO_CAST
			cipher_map_.enter(CryptoEncryption::Cipher(CryptoEncryption::CAST, CryptoEncryption::CBC), evp_factory(EVP_cast5_cbc()));
#endif
#ifndef	OPENSSL_NO_IDEA
			cipher_map_.enter(CryptoEncryption::Cipher(CryptoEncryption::IDEA, CryptoEncryption::CBC), evp_factory(EVP_idea_cbc()));
#endif
			cipher_map_.enter(CryptoEncryption::Cipher(CryptoEncryption::RC4, CryptoEncryption::Stream), evp_factory(EVP_rc4()));

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
