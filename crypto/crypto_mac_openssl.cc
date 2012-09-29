#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <common/factory.h>

#include <crypto/crypto_mac.h>

namespace {
	class InstanceEVP : public CryptoMAC::Instance {
		LogHandle log_;
		const EVP_MD *algorithm_;
		uint8_t key_[EVP_MAX_KEY_LENGTH];
		size_t key_length_;
	public:
		InstanceEVP(const EVP_MD *algorithm)
		: log_("/crypto/mac/instance/openssl"),
		  algorithm_(algorithm),
		  key_(),
		  key_length_(0)
		{ }

		~InstanceEVP()
		{ }

		unsigned size(void) const
		{
			return (EVP_MD_size(algorithm_));
		}

		Instance *clone(void) const
		{
			ASSERT(log_, key_length_ == 0);
			return (new InstanceEVP(algorithm_));
		}

		bool initialize(const Buffer *key)
		{
			if (key->length() > EVP_MAX_KEY_LENGTH)
				return (false);

			key->copyout(key_, key->length());
			key_length_ = key->length();

			return (true);
		}

		bool mac(Buffer *out, const Buffer *in)
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

			uint8_t macdata[EVP_MD_size(algorithm_)];
			unsigned maclen;
			if (HMAC(algorithm_, key_, key_length_, indata, sizeof indata, macdata, &maclen) == NULL)
				return (false);
			ASSERT(log_, maclen == sizeof macdata);
			out->append(macdata, maclen);
			return (true);
		}

		Action *submit(Buffer *in, EventCallback *cb)
		{
			Buffer out;
			if (!mac(&out, in)) {
				in->clear();
				cb->param(Event::Error);
				return (cb->schedule());
			}
			in->clear();
			cb->param(Event(Event::Done, out));
			return (cb->schedule());
		}
	};

	class MethodOpenSSL : public CryptoMAC::Method {
		LogHandle log_;
		FactoryMap<CryptoMAC::Algorithm, CryptoMAC::Instance> algorithm_map_;
	public:
		MethodOpenSSL(void)
		: CryptoMAC::Method("OpenSSL"),
		  log_("/crypto/mac/openssl"),
		  algorithm_map_()
		{
			OpenSSL_add_all_algorithms();

			factory<InstanceEVP> evp_factory;
			algorithm_map_.enter(CryptoMAC::MD5, evp_factory(EVP_md5()));
			algorithm_map_.enter(CryptoMAC::SHA1, evp_factory(EVP_sha1()));
			algorithm_map_.enter(CryptoMAC::SHA256, evp_factory(EVP_sha256()));

			/* XXX Register.  */
		}

		~MethodOpenSSL()
		{
			/* XXX Unregister.  */
		}

		std::set<CryptoMAC::Algorithm> algorithms(void) const
		{
			return (algorithm_map_.keys());
		}

		CryptoMAC::Instance *instance(CryptoMAC::Algorithm algorithm) const
		{
			return (algorithm_map_.create(algorithm));
		}
	};

	static MethodOpenSSL crypto_mac_method_openssl;
}
